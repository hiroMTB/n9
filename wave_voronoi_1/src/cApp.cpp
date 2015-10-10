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
    
    void draw_dot_wave( const Wave & wave, int start, int n);
    void draw_line_wave( const Wave & wave, int start, int n);
    void draw_voronoi();
    
    void loadColorSample( string fileName, vector<vector<Colorf>>& col );
    
    const int win_w = 4320;
    const int win_h = 1920;

    Perlin mPln;
    vector<Wave> mWave;
    vector<vector<Colorf>> mColorSample1;

    Exporter mExp;
  
    int frame = -46;
    int mFpb = 1024*4;
    float camDist = -2800;
    float amp = 400;
    
    MayaCamUI mayaCam;
    
    vector<vPoint> vPs;
    voronoi_diagram<double> vD;
};

void cApp::setup(){
    
    setFrameRate(25);
    setWindowSize( win_w*0.5, win_h*0.5 );
    setWindowPos( 0, 0 );
    gl::enableVerticalSync();
    
    mPln.setOctaves(4);
    mPln.setSeed(444);
    
    mExp.setup( win_w, win_h, 1000, GL_RGB, uf::getRenderPath(), 0 );
    uf::loadColorSample("img/Mx80_2_org_D.jpg", mColorSample1 );
    
    for( int w=0; w<8; w++ ){
        
        mWave.push_back( Wave() );
        mWave[w].create( "snd/test/3s1e_192k_" + toString(w+1) + ".wav");
        mWave[w].pos.assign( mFpb, Vec3f(0,0,0) );
        mWave[w].color.assign( mFpb, ColorAf(0.5,0.5,0.5,1) );
        
        for( int c=0; c<mWave[w].color.size(); c++ ){
            int idx = (int)mWave[w].pos[c].x % mColorSample1.size();
            int idy = (int)mWave[w].pos[c].y % mColorSample1[0].size();
            mWave[w].color[c] = mColorSample1[idx][idy];
        }
    }
    
    CameraPersp persp;
    persp.lookAt( Vec3f(0,0, camDist), Vec3f(0,0,0), Vec3f(0,-1,0) );
    persp.setAspectRatio( (double)win_w/win_h );
    persp.setNearClip(1);
    persp.setFarClip(100000);
    mayaCam.setCurrentCam( persp );
    
#ifdef RENDER
    mExp.startRender();
#endif

}

void cApp::update(){

    frame++;
    
    int nFrame = mWave[0].buf->getNumFrames();
    long audioPos = frame * mFpb;
    
    if( audioPos >= nFrame ){
        
        quit();
    }
    

    vPs.clear();
    for( int w=0; w<mWave.size(); w++ ){
        const float * ch0 = mWave[w].buf->getChannel( 0 );
        
        for ( int i=0; i<mFpb; i++) {
            float l = ch0[audioPos + i];
            mWave[w].pos[i].x = i - mFpb/2;
            mWave[w].pos[i].y = l;
            
            //if( i%2 == 1 ){
            if(1){
                float x = i - mFpb/2 + w*0.5;
                float y = l * amp;
                Vec2f n = mPln.dfBm( getElapsedFrames()*0.1 + i*0.1, 1+l*0.9);
                x += n.x*2;
                y += n.y*0.1;
                vPs.push_back( vPoint(x, y) );
            }
        }
        
        // color
        for( int i=0; i<mWave[w].color.size(); i++ ){
            ColorAf & oldColor = mWave[w].color[i];
            Vec3f noise = mPln.dfBm( getElapsedFrames()*0.1+w, mWave[w].pos[i].x*0.1, mWave[w].pos[i].y );
            noise *= 0.1;
            oldColor.r += noise.x*0.02;
            oldColor.g += noise.z*0.02;
            oldColor.b += noise.y*0.02;
            oldColor.a = 0.7;
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
        
//        for( int w=0; w<mWave.size(); w++ ){
//            for( int i=0; i<mWave[w].pos.size(); i++ ){
//                draw_dot_wave(mWave[w], i, 1);
//            }
//        }
    } mExp.end();

    gl::clear( Color(0,0,0) );
    glColor3f(1, 1, 1);
    mExp.draw();
}


void cApp::draw_voronoi(){

    gl::lineWidth(1);
//    glScalef(0.01, 0.01, 1);
    
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
                                if( randFloat()>0.85){
                                    gl::color(0.5, 0.5, 0.5);
                                    glVertex3f(x0, y0, 0);
                                    glVertex3f(x1, y1, 0);
                                }
                            }
                        }
                    }else{
                        if( 1 ){
//                            glColor3f(1, 0, 0);
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
        
        
        // draw Point

        if( 0 ){
            glPointSize(1);
            gl::color(0, 0, 1);
            glBegin(GL_POINTS);
            for( auto v : vPs )
                glVertex3f( v.x(), v.y(), 0);
            glEnd();
        }
    }
}

void cApp::draw_dot_wave( const Wave & wave, int start, int n){
    
    glPointSize(2);
    
    for( int i=0; i<n; i++){
        int id = start + i;
        const Vec3f & pos = wave.pos[id];
        const ColorAf & color = wave.color[id];
        float x = pos.x - mFpb/2;
        float y = pos.y * amp;
        glBegin( GL_POINTS );
        glColor4f( color );
        glVertex2f( x, y );
        glEnd();
    }
}

void cApp::draw_line_wave( const Wave & wave, int start, int n){
    
    glLineWidth(3);
    glBegin( GL_LINE_STRIP );
    
    for( int i=0; i<n; i++){
        int id = start + i;
        const Vec3f & pos = wave.pos[id];
        const ColorAf & color = wave.color[id];
        float x = pos.x - mFpb/2;
        float y = pos.y * amp;
        
        glColor4f( color );
        glVertex2f( x, y );
    }
    
    glEnd();
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
