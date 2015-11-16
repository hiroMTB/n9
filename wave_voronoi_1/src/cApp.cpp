#define RENDER

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/rand.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"
#include "cinder/Perlin.h"
#include "cinder/BSpline.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"

#include "boost/polygon/voronoi.hpp"
#include "boost/polygon/segment_data.hpp"

#include "mtUtil.h"
#include "Wave.h"
#include "Exporter.h"

using namespace ci;
using namespace ci::app;
using namespace std;

using boost::polygon::voronoi_builder;
using boost::polygon::voronoi_diagram;
typedef boost::polygon::point_data<float> vPoint;
typedef boost::polygon::segment_data<float> vSegment;
typedef voronoi_diagram<double>::cell_type cell_type;
typedef voronoi_diagram<double>::edge_type edge_type;
typedef voronoi_diagram<double>::vertex_type vertex_type;


class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    void draw_voronoi();
    
    const int win_w = 4320;
    const int win_h = 1920;

    vector<Perlin> mPlns;
    vector<Wave> mWave;
    vector<vPoint> vPs;
    voronoi_diagram<double> vD;
    Exporter mExp;
    fs::path assetDir;
  
    int frame       = -1;
    int mFpb        = 1920*2;   //96kHz/25fps = 3840
    float camDist   = -2800;
    float amp       = 600;
    int resolution  = 2;
    int totalMovFrame;
    int xoffset = 120;

};

void cApp::setup(){
    
    setFrameRate(25);
    setWindowSize( win_w*0.5, win_h*0.5 );
    setWindowPos( 0, 0 );
    gl::enableVerticalSync();
    
//    CameraPersp persp;
//    persp.lookAt( Vec3f(0,0, camDist), Vec3f(0,0,0), Vec3f(0,-1,0) );
//    persp.setAspectRatio( (double)win_w/win_h );
//    persp.setNearClip(1);
//    persp.setFarClip(100000);
//    mayaCam.setCurrentCam( persp );
    
    assetDir = mt::getAssetPath();
    
    for( int w=0; w<8; w++ ){
        mWave.push_back( Wave() );
        //mWave[w].create( "snd/samples/192k/3s1e_192k_" + toString(w+1) + ".wav");
        string filename = "n5_2mix_sepf_0" + to_string(w+1)+ ".aif";
        mWave[w].create( (assetDir/"snd"/"timeline"/"sepf"/filename) );
    
        mPlns.push_back(Perlin());
        mPlns[mPlns.size()-1].setOctaves(1+w%4);
        mPlns[mPlns.size()-1].setSeed(123*w*w);
    }

    int nFrame = mWave[0].buf->getNumFrames();
    totalMovFrame = nFrame / mFpb;
    
    mExp.setup( win_w, win_h, totalMovFrame, GL_RGB, mt::getRenderPath(), 0 );
    
#ifdef RENDER
    mExp.startRender();
#endif

}

void cApp::update(){

    frame++;
    
    int nFrame = mWave[0].buf->getNumFrames();
    long audioPos = frame * mFpb;
    
    if( audioPos >= nFrame ){
        cout << "Quit App, frame: " << frame << "\n"
        << "audio pos: " << audioPos << ", audio total frame : " << nFrame << endl;
        quit();
    }
    

    vPs.clear();
    
    float canvasWidth = mExp.mFbo.getWidth()-xoffset*2;
    float sampleDistance = (float)canvasWidth/mFpb;
    
    for( int w=0; w<mWave.size(); w++ ){
        const float * ch0 = mWave[w].buf->getChannel( 0 );
        //const float * ch1 = mWave[w].buf->getChannel( 1 );
        
        for ( int i=0; i<mFpb; i++) {            
            if( i!=1 && i%resolution==0 ){
                float l = ch0[audioPos + i];
                //float r = ch1[audioPos + i];
                
                float x1 = i * mExp.mFbo.getWidth()/canvasWidth;
                x1 -= mExp.mFbo.getWidth()/2;
                x1 += xoffset;
                
                x1 += sampleDistance * w/mWave.size();
                //float x2 = i - mFpb/2 + sampleDistance*0.5;
                
                float y1 = l * amp;
                y1 *= (w+2)*0.4;

                //float y2 = r * amp - 10;
                Vec3f n1 = mPlns[w].dfBm( frame*0.1*w, w*i*0.2, 1+l*0.5);
                //Vec2f n2 = mPln.dfBm( getElapsedFrames()*10 + i*3, 1+r*0.91);
                x1 += n1.x*n1.z*8;
                y1 += n1.y*n1.x*8;
                
                float yoffset = 16;
                y1 -= yoffset*(mWave.size()-1)/2;
                y1 += w * yoffset;
                
                //x2 += n2.x*2;
                //y2 += n2.y*2;

                if( mPlns[w].noise(x1*0.1, y1 ) > 0.95  ){
                    y1 *= randFloat(2,6);
                }

                vPs.push_back( vPoint(x1, y1) );
                //vPs.push_back( vPoint(x2, y2) );
            }
        }
    }
    
    vD.clear();
    boost::polygon::construct_voronoi( vPs.begin(), vPs.end(), &vD);
}

void cApp::draw(){

    gl::enableAlphaBlending();
    
    mExp.begin(); {
        gl::clear( Color(0,0,0) );
        glPushMatrix();
        gl::translate(mExp.mFbo.getWidth()/2, mExp.mFbo.getHeight()/2);
        draw_voronoi();
        glPopMatrix();
        
        //if( !mExp.bRender && !mExp.bSnap ){
            glPointSize(4);
            gl::color(1, 0, 0, 1);
            //glBegin(GL_POINTS);
            glBegin(GL_LINES);
            glVertex3f( 120, win_h/2, 0 );
            glVertex3f( win_w-120, win_h/2, 0 );
            glEnd();
        //}
    } mExp.end();

    gl::clear( Color(0,0,0) );
    glColor3f(1, 1, 1);
    mExp.draw();
    
}

void cApp::draw_voronoi(){

    if( 1 ){
        gl::lineWidth(1);
        gl::begin(GL_LINES);
        for( int j=0; j<vPs.size(); j++ ){
            
            voronoi_diagram<double>::const_cell_iterator it = vD.cells().begin();
            for (; it!= vD.cells().end(); ++it) {
                
                const cell_type& cell = *it;
                const edge_type* edge = cell.incident_edge();
                
                do{
                    if(edge->is_primary()){
                        if( edge->is_finite() ){
                            if (edge->cell()->source_index() < edge->twin()->cell()->source_index()){
                                float x0 = edge->vertex0()->x();
                                float y0 = edge->vertex0()->y();
                                float x1 = edge->vertex1()->x();
                                float y1 = edge->vertex1()->y();
                                
                                float limity = 800;
                                float limitx = 4000;
                                bool xOver = ( abs(x0)>limitx || abs(x1)>limitx );
                                bool yOver = ( abs(y0)>limity || abs(y1)>limity );
                                
                                bool draw = !xOver && !yOver;
                                
                                if(draw){
                                    gl::color(0.7, 0.7, 0.7, 0.5);
                                    glVertex3f(x0, y0, 0);
                                    glVertex3f(x1, y1, 0);
                                }else{
                                    if( randFloat()>0.9){
                                        gl::color(0.5, 0.5, 0.5, 0.5);
                                        glVertex3f(x0, y0, 0);
                                        glVertex3f(x1, y1, 0);
                                    }
                                }
                            }
                        }else{
                            if( 0 ){
                                const vertex_type * v0 = edge->vertex0();
                                if( v0 ){
                                    vPoint p1 = vPs[edge->cell()->source_index()];
                                    vPoint p2 = vPs[edge->twin()->cell()->source_index()];
                                    float x0 = edge->vertex0()->x();
                                    float y0 = edge->vertex0()->y();
                                    float end_x = (p1.y() - p2.y()) * win_w;
                                    float end_y = (p1.x() - p2.x()) * -win_w;
                                    glVertex3f(x0, y0, 0);
                                    glVertex3f(end_x, end_y, 0);
                                }
                            }
                        }
                    }
                    edge = edge->next();
                }while (edge != cell.incident_edge());
            }
            glEnd();
        }
    }
    
    // draw Point
    if( 0 ){
        glPointSize(3);
        gl::color(1.0, 0.1, 0.1, 0.7);
        glBegin(GL_POINTS);
        for( auto v : vPs )
            glVertex3f( v.x(), v.y(), 0);
        glEnd();
    }
    

}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
