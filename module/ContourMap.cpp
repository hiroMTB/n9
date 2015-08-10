#include "ContourMap.h"
#include "cinder/app/AppBasic.h"
#include "cinder/cairo/Cairo.h"
#include "cinder/gl/Texture.h"

using namespace ci;
using namespace ci::app;

ContourMap::ContourMap(){
}

void ContourMap::setImage( ci::Surface32f sur, bool convert2gray=true, cv::Size blurSize=cv::Size(2,2) ){
    
    gl::Texture mTex = gl::Texture( sur );
    
    input = toOcv( sur );
    
    if( convert2gray )
        cv::cvtColor( input, input, CV_RGB2GRAY );
    
    cv::blur( input, input, blurSize );
}

void ContourMap::addContour( float threshold, int filterType, bool output_threshold_image ){
    ContourGroup cg;
    
    /* 
     *      findContour pre process
     *
     *      cv::threshold   - ok
     *      cv::Canny       - error
     *      cv::Sobel       - ?
     */
    
    cv::threshold( input, thresh, threshold, 1.0, CV_THRESH_BINARY );

    if( output_threshold_image ){
        Surface32f thresh_sur = fromOcv( thresh );
        createDirectories( "../../out/threshold" );
        writeImage( "../../out/threshold/" + (toString(threshold)+".png"), thresh_sur );
    }
 
    thresh.convertTo(thresh, CV_32SC1);
    
    if( thresh.type() == CV_32SC1 ){
        vector<cv::Vec4i> hierarchy;
        vector<vector<cv::Point>> cg2i;
        cv::findContours( thresh, cg2i, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE, cv::Point(0, 0));
        
        for( int i=0; i<cg2i.size(); i++ ){
            Contour c;
            c.assign( cg2i[i].size(), cv::Point2f(0,0) );
            for( int j=0; j<cg2i[i].size(); j++ ){
                c[j] = cg2i[i][j];
            }
            cg.push_back(c);
        }
        cg2i.clear();
        
        ContourGroup::iterator itr = cg.begin();

        /*
         *  Filterling Contour to aboid duplicated line
         */
        switch (filterType) {

            // remove odd contour
            case 0:
                for(int i=0; itr!=cg.end(); i++){
                    if( i%2==1 )
                        itr = cg.erase(itr);
                    else
                        ++itr;
                }
                break;
                
            // remove outer contour
            case 1:
            {
                vector<cv::Vec4i>::iterator itrh = hierarchy.begin();
                for( ; itr!=cg.end(); ){
                    // look for outer contour
                    if ( (*itrh)[2] == -1 ){
                        itr = cg.erase(itr);
                        itrh = hierarchy.erase(itrh);
                    }else{
                        ++itr;
                        ++itrh;
                    }
                }
            }
                break;

            // remove inner contour
            case 2:
            {
                vector<cv::Vec4i>::iterator itrh = hierarchy.begin();
                for( ; itr!=cg.end(); ){
                    // look for outer contour
                    if ( (*itrh)[3] == -1 ){
                        itr = cg.erase(itr);
                        itrh = hierarchy.erase(itrh);
                    }else{
                        ++itr;
                        ++itrh;
                    }
                }
            }
                break;

        }
        
        
        mCMapData.push_back( cg );
    }else{
        cout << "error : wrong image format " << endl;
    }
    

    
}

void ContourMap::drawContourAll(){

    glBegin( GL_POINTS );
    for( auto & cg : mCMapData )
        for( auto & c : cg )
            for( auto & p : c )
                gl::vertex( fromOcv(p) );

    glEnd();
}

void ContourMap::drawContourGroup( int whichThreshold ){
    
    if( mCMapData.size() <= whichThreshold )
        return;
    
    glBegin( GL_POINTS );
    for( auto & c : mCMapData[whichThreshold] )
        for( auto p : c )
            gl::vertex( fromOcv(p) );
    
    glEnd();
    
}

void ContourMap::exportContour( string path, string fileType="eps" ){
 
    int w = 3000;
    int h = 3000;

    for ( auto & cg : mCMapData ){
        int nMap = &cg - &mCMapData[0];
 
        string pathn = path + "_" + toString(nMap);
        cairo::Context ctx;
        if( fileType == "svg" ){
            ctx = cairo::Context( cairo::SurfaceSvg( fs::path(pathn+=".svg"), w, h ) );
        }else if( fileType == "eps" ){
            ctx = cairo::Context( cairo::SurfaceEps( fs::path(pathn+=".eps"), w, h ) );
        }else if( fileType == "pdf" ){
            ctx = cairo::Context( cairo::SurfacePdf( fs::path(pathn+=".pdf"), w, h ) );
        }else{
            cout << "Error : exportContour, strange fileType of " << fileType << endl;
            return;
        }
        
        cout << "exportContour : " << pathn << endl;

        ctx.setSource( ColorAf(1,1,1,1) );
        ctx.paint();
        ctx.stroke();
        
        ctx.setSource( ColorAf(0,0,0,1) );
        ctx.setLineWidth(1);
        for( auto & c : cg ){
            ctx.newPath();
            for( auto & p : c ){
                ctx.lineTo( p.x, p.y );
            }
            ctx.closePath();
            ctx.stroke();
        }
    }
}