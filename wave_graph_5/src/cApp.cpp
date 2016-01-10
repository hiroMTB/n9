#define RENDER

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"
#include "cinder/ImageIo.h"
#include "cinder/Perlin.h"
#include "cinder/rand.h"
#include "cinder/BSpline.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"

#include "mtUtil.h"
#include "mtSvg.h"
#include "Wave.h"
#include "Exporter.h"
#include "Axis.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    void draw_wave();
    
    const int win_w = 4320;
    const int win_h = 1920;

    Exporter mExp;
    fs::path assetDir;
  
    Wave m2mix;
    
    int frame       = -1;
    int mFpb        = 1920;   //48kHz/25fps = 1920
    float amp       = 250;
    int resolution  = 1;
    int totalMovFrame;
    int xoffset = 120;
    
    mtSvg guide;
    bool bInit = false;
};

void cApp::setup(){
    
    setFrameRate( 25 );
    setWindowSize( win_w*0.5, win_h*0.5 );
    setWindowPos( 0, 0 );
    gl::enableVerticalSync();
    assetDir = mt::getAssetPath();
    guide.load( assetDir/"svg"/"n5_guide.svg" );

    m2mix.create( (assetDir/"snd"/"timeline"/"n5_2mix_192k.aif"), mFpb );

    int nFrame = m2mix.buf->getNumFrames();
    totalMovFrame = nFrame / mFpb;
    
    mExp.setup( win_w, win_h, totalMovFrame, GL_RGB, mt::getRenderPath(), 0 );
    
#ifdef RENDER
    mExp.startRender();
#endif

}

void cApp::update(){

    frame++;
    
    int nFrame = m2mix.buf->getNumFrames();
    long audioPos = frame * mFpb;
    
    if( audioPos >= nFrame ){
        cout << "Quit App, frame: " << frame << "\n"
        << "audio pos: " << audioPos << ", audio total frame : " << nFrame << endl;
        quit();
    }
    
    m2mix.updatePos( audioPos );
    
}

void cApp::draw(){

    gl::enableAlphaBlending();
    
    mExp.beginPersp(); {
        gl::clear( Color(0,0,0) );
        glPushMatrix();
        gl::translate(mExp.mFbo.getWidth()/2, mExp.mFbo.getHeight()/2);
        draw_wave();
        glPopMatrix();
        
        if( bInit )
        {
            guide.draw();
            bInit = true;
        }
        
        n5::drawAxes();
        
    } mExp.end();

    gl::clear( Color(0,0,0) );
    glColor3f(1, 1, 1);
    mExp.draw();
    
}

void cApp::draw_wave(){
  
    // draw Point
    {
        int sampleNum = mFpb/resolution;
        
        for( int c=0; c<m2mix.numCh; c++ ){
            
            for ( int i=0; i<sampleNum; i++) {
                
            }
        }
    }
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )

