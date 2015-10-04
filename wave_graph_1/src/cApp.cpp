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

    void keyDown( KeyEvent event );
    void mouseMove( MouseEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void resize();
    void loadColorSample( string fileName, vector<vector<Colorf>>& col );
    
    const int win_w = 4320;
    const int win_h = 1920;

    Perlin mPln;
    
    vector<Wave> mWaves;
    vector<vector<Colorf>> mColorSample1;

    Exporter mExp;
  
    int frame = -1;
    
    float fftWaveSamplingRate = 6000;
    int mFpb = 2048*2;

};

void cApp::setup(){
    
    setFrameRate(25);
    setWindowSize( win_w*0.5, win_h*0.5 );
    setWindowPos( 0, 0 );
    gl::enableVerticalSync();
    
    mPln.setOctaves(4);
    mPln.setSeed(444);
    
    {
        // Visual Setup
        mExp.setup( win_w, win_h, 3000, GL_RGB, uf::getRenderPath(), 0 );
        uf::loadColorSample("img/Mx80_2_org_C.jpg", mColorSample1 );
    }
    
    {
        mWaves.assign( 8, Wave() );
        
        for( int i=0; i<mWaves.size(); i++ ){
            mWaves[i].create( "snd/test/3s1e_192k_" + toString(i+1)+ ".wav" );
            mWaves[i].pos.assign( mFpb, Vec3f(0,0,0) );
            mWaves[i].color.assign( mFpb, ColorAf(0.5,0.5,0.5,1) );
        }
    }
    
#ifdef RENDER
    mExp.startRender();
#endif

}

void cApp::update(){

    
    frame++;
    
    {
        int audioPos = frame * mFpb;
        Vec3f scale( (float)(win_w-100)/mFpb, -300, 0);
        
        for( int w=0; w<mWaves.size(); w++ ){
            
            const float * ch0 = mWaves[w].buf->getChannel( 0 );
            
            for ( int i=0; i<mFpb; i++) {
                float l = ch0[audioPos + i];
                Vec3f newL = Vec3f(i*scale.x, l*scale.y, scale.z);
                mWaves[w].pos[i] = newL;
            }
            
            // color
            for( int i=0; i<mWaves[w].color.size(); i++ ){
                float noise = mPln.noise( frame*0.1, w+i*0.1 );
                noise = (noise+1.0f) * 0.4f;
                int y = noise * mColorSample1.size()-1;
                y = MAX(y, 0);
                y = MIN(y, mColorSample1.size());
                
                Colorf & newColor = mColorSample1[getElapsedFrames()][y];
                ColorAf & oldColor = mWaves[w].color[i];
                
                oldColor.r = oldColor.r*0.88 + newColor.r*0.12;
                oldColor.g = oldColor.g*0.88 + newColor.g*0.12;
                oldColor.b = oldColor.b*0.88 + newColor.r*0.12;
                oldColor.a = 0.8;
            }
        }
    }

}

void cApp::draw(){

    glPointSize(2);
    glLineWidth(1);

    gl::enableAlphaBlending();
    
    mExp.begin();
    gl::clear( Color(0,0,0) );
    {
        
        // graph line
        
        float center_x = win_w *0.5;
        float center_y = win_h * 0.5;
        
        float scale_x = (win_w-100)/2;
        float scale_y = win_h/2 * 0.5;
        
        glPushMatrix();
        glTranslatef( center_x, center_y, 0);
        
        glColor4f(1, 0, 0, 0.8);
        
        {
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
        
        glPopMatrix();
        
        
        glTranslatef( 50, win_h/2 , 0 );
        
        // Wave
        for( int w=mWaves.size()-1; 0<=w; w-- ){

            for( int i=0; i<mWaves[w].pos.size(); i++ ){
                glPointSize( 1 );
                glColor4f( mWaves[w].color[i].r, mWaves[w].color[i].g, mWaves[w].color[i].b, 0.9 );
                glBegin( GL_POINTS );
                glVertex2f( mWaves[w].pos[i].x,  mWaves[w].pos[i].y );
                glEnd();
            }
        }
    }
    mExp.end();
    
    gl::clear( Color(0,0,0) );
    glColor3f(1, 1, 1);
    mExp.draw();
    
}

void cApp::keyDown( KeyEvent event ){}
void cApp::mouseDown( MouseEvent event ){}
void cApp::mouseMove( MouseEvent event ){}
void cApp::mouseDrag( MouseEvent event ){}
void cApp::resize(){}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
