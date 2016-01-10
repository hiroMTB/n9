#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Camera.h"
#include "cinder/gl/Texture.h"
#include "CinderOpenCv.h"
#include "cinder/cairo/Cairo.h"
#include "mtUtil.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();

    void keyDown( KeyEvent event );
    void mouseMove( MouseEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void resize();

    Vec2f mMousePos;
    MayaCamUI mMayaCam;

    cv::Mat input;
    cv::Mat thresh;
    typedef vector<cv::Point> Contour;
    typedef vector<Contour> ContourGroup;

    vector<ContourGroup> contourMap;
    vector<vector<cv::Vec4i>> hierarchyMap;
};

void cApp::setup(){
    setWindowSize( 1920, 1080 );
    setWindowPos( 0, 0 );
    
    CameraPersp cam;
    cam.setEyePoint( Vec3f(1500, 1500, 3000.0f) );
    cam.setCenterOfInterestPoint( Vec3f(1500, 1500, 0.0) );
    cam.setPerspective( 60.0f, getWindowAspectRatio(), 1.0f, 10000.0f );
    mMayaCam.setCurrentCam( cam );
    
    
    // CONTOUR
    // loadAsset search parent directry, on up 5 level
    fs::path assetPath = mt::getAssetPath();
    //Surface32f sur( loadImage( (assetPath/("img/geo/landsat/LC80770172015167LGN00/s/LC80770172015167LGN00_B7_s.TIF"))) );
    Surface32f sur( loadImage( (assetPath/("img/geo/alos/N031E107/AVERAGE/N031E107_AVE_DSM.tif"))) );
    gl::Texture mTex = gl::Texture( sur );
    input = toOcv( sur );
    cv::cvtColor( input, input, CV_RGB2GRAY );
    cv::blur( input, input, cv::Size(6,6) );

    float threshold = 0;
    for( int j=0; j<10; j++ ){
        ContourGroup cg;
        vector<cv::Vec4i> hierarchy;
        
        threshold = j*0.01 + 0.01f;
        
        cv::threshold( input, thresh, threshold, 1.0, CV_THRESH_BINARY );
        
        cv::Mat thresh32sc1;
        thresh.convertTo( thresh32sc1, CV_32SC1 );
        cv::findContours( thresh32sc1.clone(), cg, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE, cv::Point(0, 0) );

        // store
        contourMap.push_back( cg );
        hierarchyMap.push_back( hierarchy );
        
        // Export eps
        if( 0 ){
            cairo::Context ctx;
            fs::path outpath( "../../out" );
            createDirectories( outpath );
            ctx = cairo::Context( cairo::SurfaceSvg( fs::path("../../out/contour_" + to_string(threshold) + ".svg"), 3000, 3000 ) );
            //ctx = cairo::Context( cairo::SurfacePdf( fs::path("../../../out/contour_" + to_string(threshold) + ".pdf"), 3000, 3000 ) );
            
            ctx.setLineWidth( 1 );
            ctx.setSource( ColorAf(0,0,1,1) );
            ctx.rectangle( 10, 10, 3000-20, 3000-20 );
            ctx.stroke();
            
            ctx.setSource( ColorAf(0,0,0,1) );
            ctx.newPath();
            for( int i=0; i<cg.size(); i++ ){
                if( i%2==1 )  continue;
//                bool haveParent = hierarchy[i][2] < 0;
//                bool haveChild = hierarchy[i][3] < 0;
//                
//                if( haveParent && haveChild )
//                    ctx.setSource( Colorf(0.5,0.5,0.5) );
//                else if( haveParent && !haveChild ){
//                    ctx.setSource( Colorf(1,0,0) );
//                    //continue;
//                }else if( !haveParent && haveChild ){
//                    ctx.setSource( Colorf(0,0,1) );
//                    //continue;
//                }else if( !haveParent && !haveChild ){
//                    ctx.setSource( Colorf(0,0,0) );
//                }else {
//                    ctx.setSource( Colorf(1,0,0.3) );
//                }
                
                ctx.newSubPath();
                if( cg[i].size() >=5 ){
                    for( auto & p : cg[i] ){
                        ctx.lineTo( p.x, p.y );
                    }
                }
                ctx.closePath();
                //ctx.stroke();
                
            }
            ctx.stroke();
        }
    }
}

void cApp::update(){
}

void cApp::draw(){
    
    gl::pushMatrices();
    gl::setMatrices( mMayaCam.getCamera() );
    gl::clear( Color(1,1,1) );
    gl::color( Color( 0,0,0) );
    
    glBegin( GL_POINTS );
    for(int j=0; j<contourMap.size(); j++ ){
        ContourGroup &cg = contourMap[j];
        
        for( int i=0; i<cg.size(); i++ ){
            Contour & c = cg[i];
            if( hierarchyMap[j][i][2] == -1 ){
                gl::color(1, 0, 0);
            }else if( hierarchyMap[j][i][3] == -1 ){
                gl::color(0, 1, 0);
            }
            
            for( int j=0; j<c.size(); j++){
                gl::vertex( fromOcv(c[j]) );
            }
        }
    }
    
    glEnd();

    gl::popMatrices();

}

void cApp::keyDown( KeyEvent event ){
    
}

void cApp::mouseDown( MouseEvent event ){
    mMayaCam.mouseDown( event.getPos() );
}

void cApp::mouseMove( MouseEvent event ){
    mMousePos = event.getPos();
}

void cApp::mouseDrag( MouseEvent event ){
    mMousePos = event.getPos();
    mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void cApp::resize(){
    CameraPersp cam = mMayaCam.getCamera();
    cam.setAspectRatio( getWindowAspectRatio() );
    mMayaCam.setCurrentCam( cam );
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
