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

#include "ufUtil.h"
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

    Perlin mPln;
    vector<Wave> mWave;
    MayaCamUI mayaCam;
    vector<vPoint> vPs;
    voronoi_diagram<double> vD;
    Exporter mExp;
  
    int frame       = 185-1;
    int mFpb        = 1024*4;
    float camDist   = -2800;
    float amp       = 400;
    int resolution  = 3;
};

void cApp::setup(){
    
    setFrameRate(25);
    setWindowSize( win_w*0.5, win_h*0.5 );
    setWindowPos( 0, 0 );
    gl::enableVerticalSync();
    
    mPln.setOctaves(4);
    mPln.setSeed(444);
    
    CameraPersp persp;
    persp.lookAt( Vec3f(0,0, camDist), Vec3f(0,0,0), Vec3f(0,-1,0) );
    persp.setAspectRatio( (double)win_w/win_h );
    persp.setNearClip(1);
    persp.setFarClip(100000);
    mayaCam.setCurrentCam( persp );
    
    
    for( int w=0; w<8; w++ ){
        mWave.push_back( Wave() );
        mWave[w].create( "snd/samples/192k/3s1e_192k_" + toString(w+1) + ".wav");
    }
    
    mExp.setup( win_w, win_h, 1000, GL_RGB, uf::getRenderPath(), 0 );
    
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
    for( int w=0; w<mWave.size(); w++ ){
        const float * ch0 = mWave[w].buf->getChannel( 0 );
        
        for ( int i=0; i<mFpb; i++) {            
            if( i!=1 && i%resolution==0 ){
                float l = ch0[audioPos + i];
                float x = i - mFpb/2 + w*0.5;
                float y = l * amp;
                Vec2f n = mPln.dfBm( getElapsedFrames()*0.1 + i*0.1, 1+l*0.9);
                x += n.x*2;
                y += n.y*0.1;
                vPs.push_back( vPoint(x, y) );
            }
        }
    }
    
    vD.clear();
    boost::polygon::construct_voronoi( vPs.begin(), vPs.end(), &vD);
}

void cApp::draw(){

    gl::enableAlphaBlending();
    
    mExp.begin( mayaCam.getCamera() ); {
        gl::clear( Color(0,0,0) );
        draw_voronoi();
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
                                    gl::color(0.7, 0.7, 0.7, 0.7);
                                    glVertex3f(x0, y0, 0);
                                    glVertex3f(x1, y1, 0);
                                }else{
                                    if( randFloat()>0.9){
                                        gl::color(0.5, 0.5, 0.5);
                                        glVertex3f(x0, y0, 0);
                                        glVertex3f(x1, y1, 0);
                                    }
                                }
                            }
                        }else{
                            if( 1 ){
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
        glPointSize(2);
        gl::color(1.0, 0.1, 0.1, 0.7);
        glBegin(GL_POINTS);
        for( auto v : vPs )
            glVertex3f( v.x(), v.y(), 0);
        glEnd();
    }
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
