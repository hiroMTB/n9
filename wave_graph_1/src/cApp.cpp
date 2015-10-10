//#define RENDER

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
#include "ufUtil.h"
#include "Wave.h"
#include "Exporter.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    void draw_plot();
    void draw_line();
    void draw_perlin_wave();
    void draw_3d();
    void draw_angle();
    void loadColorSample( string fileName, vector<vector<Colorf>>& col );
    
    const int win_w = 4320;
    const int win_h = 1920;

    Perlin mPln;
    vector<Wave> mWaves;
    vector<vector<Colorf>> mColorSample1;

    Exporter mExp;
  
    int frame = -1;
    float fftWaveSamplingRate = 6000;
    int mFpb = 2048*2*2;
    
    MayaCamUI mayaCam;
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
    
    mWaves.assign( 1, Wave() );
    
    for( int i=0; i<mWaves.size(); i++ ){
        mWaves[i].create( "snd/samples/192k/beach_ostend_01__192k.wav");
        mWaves[i].pos.assign( mFpb, Vec3f(0,0,0) );
        mWaves[i].color.assign( mFpb, ColorAf(0.5,0.5,0.5,1) );
        
        for( int c=0; c<mWaves[i].color.size(); c++ ){
            int idx = (int)mWaves[i].pos[c].x % mColorSample1.size();
            int idy = (int)mWaves[i].pos[c].y % mColorSample1[0].size();
            mWaves[i].color[c] = mColorSample1[idx][idy];
        }
    }

    CameraPersp persp;
    persp.setAspectRatio( win_w/win_h);
    persp.setNearClip(1);
    persp.setFarClip(100000);
    persp.lookAt( Vec3f(0,0,-1200), Vec3f(0,0,0), Vec3f(0,1,0) );
    mayaCam.setCurrentCam( persp );
    
#ifdef RENDER
    mExp.startRender();
#endif

}

void cApp::update(){

    frame++;
    
    int audioPos = frame * mFpb;
    for( int w=0; w<mWaves.size(); w++ ){
        
        const float * ch0 = mWaves[w].buf->getChannel( 0 );
        
        for ( int i=0; i<mFpb; i++) {
            float l = ch0[audioPos + i];
            Vec3f newL = Vec3f(i, l, 0);
            mWaves[w].pos[i] = newL;
        }

        // color
        for( int i=0; i<mWaves[w].color.size(); i++ ){
            ColorAf & oldColor = mWaves[w].color[i];
            Vec3f noise = mPln.dfBm( getElapsedFrames()*0.2, mWaves[w].pos[i].x*0.2, mWaves[w].pos[i].y*0.2 );
            noise *= 0.1;
            oldColor.r += noise.x;
            oldColor.g += noise.z;
            oldColor.b += noise.y;
            oldColor.a = 0.8;
        }
    }
}

void cApp::draw(){

    gl::enableAlphaBlending();
    glPointSize(2);

    mExp.begin( mayaCam.getCamera() ); {
        gl::clear( Color(0,0,0) );
        //draw_perlin_wave();
        //draw_line();
        draw_angle();
        //draw_3d();
    } mExp.end();
    
    gl::clear( Color(0,0,0) );
    glColor3f(1, 1, 1);
    mExp.draw();
}

void cApp::draw_perlin_wave(){
    
    glPushMatrix(); {
        for( int w=mWaves.size()-1; 0<=w; w-- ){
            for( int i=0; i<mWaves[w].pos.size(); i++ ){

                float x = mWaves[w].pos[i].x;
                float y = mWaves[w].pos[i].y;
                
                Vec3f v = mPln.dfBm(getElapsedFrames()*2, y, x) * 300;
                v.x *= 2.0;
                glBegin( GL_POINTS );
                glColor4f( mWaves[w].color[i] );
                glVertex3f( v );
                glEnd();
            }
        }
    } glPopMatrix();
}

void cApp::draw_line(){
    
    float line_height = 2;

    glColor3f(1, 0, 0);
    glLineWidth(1);

    glPushMatrix(); {
        for( int w=mWaves.size()-1; 0<=w; w-- ){
            for( int i=0; i<mWaves[w].pos.size(); i++ ){
                
                float x = mWaves[w].pos[i].x;
                float y = mWaves[w].pos[i].y;
                
                Vec2f v1(x*4+y*20, line_height);
                Vec2f v2(x*4+y*20, -line_height);
                
                glBegin( GL_LINES );
                glColor4f( mWaves[w].color[i] );
                glVertex2f( v1 );
                glVertex2f( v2 );
                glEnd();
            }
        }
    } glPopMatrix();
}


void cApp::draw_angle(){
    float line_height = 100;
    
    glLineWidth(1);
    
    glPushMatrix(); {
        for( int w=mWaves.size()-1; 0<=w; w-- ){
            for( int i=0; i<mWaves[w].pos.size(); i++ ){
                
                float x = mWaves[w].pos[i].x - mFpb/2;
                float y = mWaves[w].pos[i].y;
                
                x *= 4;
                
                Vec2f stick(0, line_height + y*300);
                stick.rotate( y*90.0 );
                Vec2f v1(x, y*400);
                Vec2f v2(x+stick.x, stick.y);
                
                glBegin( GL_LINES );
                glColor4f( mWaves[w].color[i] );
                glVertex2f( v1 );
                glVertex2f( v2 );
                glEnd();
            }
        }
    } glPopMatrix();
}

void cApp::draw_3d(){
    
    glColor3f(1, 0, 0);
    glLineWidth(1);
    
    glPushMatrix(); {
        for( int w=mWaves.size()-1; 0<=w; w-- ){
            int index = 0;
            while( index+2 < mWaves[w].pos.size() ){
                
                float x = mWaves[w].pos[index+0].y;
                float y = mWaves[w].pos[index+1].y;
                float z = mWaves[w].pos[index+2].y;
                
                Vec3f v(x, y, z);
                v *= 300;
                
                glBegin( GL_POINTS );
                glVertex3f( v );
                glEnd();
                
                index+=3;
            }
        }
    } glPopMatrix();
}

void cApp::draw_plot(){

    glPushMatrix(); {
        // graph line
        float center_x = win_w *0.5;
        float center_y = win_h * 0.5;
        float scale_x = win_w/2;
        float scale_y = win_h/2 * 0.5;
        
        glTranslatef( center_x, center_y, 0);
        glColor4f(1, 0, 0, 0.8);
        
        if(1){
            // horizontal line
            int nStep = 10;
            float step = 10.0 / nStep;
            
            for( int i=0; i<nStep; i++ ){
                float plot;
                
                if( false ){
                    plot = 1.0 - log10( step*(i) );
                }else{
                    plot= pow(i,2)/pow(nStep-1,2);
                }
                
                float x = scale_x * plot;
                
                // horizontal line
                glBegin( GL_LINES );
                glVertex3f(  x,  scale_y, 0);
                glVertex3f(  x, -scale_y, 0);
                glVertex3f( -x,  scale_y, 0);
                glVertex3f( -x, -scale_y, 0);
                glEnd();
            }
        }
        
        if(0){
            // vertical line
            int nStep = 5;
            float step = 10.0 / nStep;
            
            for( int i=0; i<nStep; i++ ){
                float plot;
                
                if( false ){
                    plot = 1.0 - log10( step*(i) );
                }else{
                    plot= pow(i,2)/pow(nStep-1,2);
                }
                
                float y = scale_y * plot;
                
                // vertical line
                glBegin( GL_LINES );
                glVertex3f(  scale_x,  y, 0);
                glVertex3f( -scale_x,  y, 0);
                glVertex3f(  scale_x, -y, 0);
                glVertex3f( -scale_x, -y, 0);
                glEnd();
            }
        }
    } glPopMatrix();
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
