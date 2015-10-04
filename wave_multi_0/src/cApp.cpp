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
    
    Wave orig;
    vector<Wave> mWaves;
    vector<vector<Colorf>> mColorSample1;
    vector<vector<Colorf>> mColorSample2;

    Exporter mExp;
  
    int frame = -1;
    int additional = 0;

    vector<Vec2f> adder;
    vector<ColorAf> colorAdder;
    
    float fftWaveSamplingRate = 6000;
    float origWaveSampingRate = 192000;
    
    int mFpb = 2048*2;
    int fpbOrig =mFpb * origWaveSampingRate/fftWaveSamplingRate;

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
        uf::loadColorSample("img/Mx80_2_org_B.jpg", mColorSample1 );
        uf::loadColorSample("img/Mx80_2_org_B.jpg", mColorSample2 );
    }
    
    {
        mWaves.assign( 16, Wave() );
        
        for( int i=0; i<mWaves.size(); i++ ){
            mWaves[i].create( "snd/fftwave_1/smooth_0.5/LF/fftSize_32/fft_" + toString(i)+ ".wav" );
            mWaves[i].pos.assign( mFpb, Vec3f(0,0,0) );
            mWaves[i].color.assign( mFpb, ColorAf(0,0,0,1) );
        }

        orig.create( "snd/test/3s1e_192k.wav" );
        orig.pos.assign( fpbOrig, Vec3f(0,0,0) );
        orig.color.assign( fpbOrig, ColorAf(0,0,0,1) );
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
                Vec2f noise = mPln.dfBm( frame*0.1, w+i*0.1 );
                noise = (noise+Vec2f(1.0f, 1.0f)) * 0.4f;
                int x = noise.x*mColorSample1[0].size()-1;
                int y = noise.y*mColorSample1.size()-1;
                x = MAX(x, 0);
                y = MAX(y, 0);
                x = MIN(x, mColorSample1[0].size());
                y = MIN(y, mColorSample1.size());
                
                mWaves[w].color[i] =  mColorSample1[x][y];
            }
        }
    }
    
    {
        // update original wave
        int audioPos = frame * fpbOrig;
        const float * ch0 = orig.buf->getChannel( 0 );
        
        Vec3f scale( (float)(win_w-100)/fpbOrig, -300, 0);
        
        for ( int i=0; i<fpbOrig; i++) {
            float l = ch0[audioPos + i];
            Vec3f newL = Vec3f(i*scale.x, l*scale.y, scale.z);
            orig.pos[i] = newL;
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
        int yadd = win_h/(mWaves.size()+2);
        glTranslatef( 50, yadd/2, 0 );

        glTranslatef( 0, yadd , 0 );
        for( int i=0; i< orig.pos.size(); i++ ){
            glPointSize( 1 );
            glColor4f( 1,0,0,0.9 );
            glBegin( GL_POINTS );
            glVertex2f( orig.pos[i].x, orig.pos[i].y );
            glEnd();
        }
        
        // fft Wave
        for( int w=mWaves.size()-1; 0<=w; w-- ){
            glTranslatef( 0, yadd , 0 );

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

void cApp::keyDown( KeyEvent event ){
}

void cApp::mouseDown( MouseEvent event ){
}

void cApp::mouseMove( MouseEvent event ){
}

void cApp::mouseDrag( MouseEvent event ){
}

void cApp::resize(){
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
