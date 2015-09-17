#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "cinder/audio/Context.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"

#include "ufUtil.h"

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
    
    audio::BufferPlayerNodeRef mPlayer;
  	audio::MonitorSpectralNodeRef mMonitor;
    audio::BufferRef buf;
    
    vector<float> mSpc;

};

void cApp::setup(){
    
    setFrameRate(25);
    setWindowSize( 1920, 1080 );
    setWindowPos( 0, 0 );
    gl::enableVerticalSync();
    
    auto ctx = audio::Context::master();
    
    audio::SourceFileRef sourceFile = audio::load( loadAsset( "snd/test/3s1e.wav" ), ctx->getSampleRate() );
    buf = sourceFile->loadBuffer();

    mPlayer = ctx->makeNode( new audio::BufferPlayerNode(buf) );
    auto monitorFormat = audio::MonitorSpectralNode::Format().fftSize( 2048 ).windowSize( 1024 );
    mMonitor = ctx->makeNode( new audio::MonitorSpectralNode( monitorFormat ) );

    mPlayer >> ctx->getOutput();
    mPlayer >> mMonitor;

    mPlayer->start();
    mPlayer->seekToTime( 100 );
    
    ctx->enable();

}

void cApp::update(){
    
    mSpc = mMonitor->getMagSpectrum();
}

void cApp::draw(){

    auto ctx = audio::Context::master();
    const audio::Buffer * cBuffer = ctx->getOutput()->getInternalBuffer();

    int nFrames = cBuffer->getNumFrames();
    const float * ch0 = cBuffer->getChannel( 0 );
    const float * ch1 = cBuffer->getChannel( 1 );

    gl::clear( Color(0,0,0) );
    
    glPushMatrix(); {
        glTranslatef( 50, getWindowHeight()/2, 0 );
        glPointSize(1);
        glColor4f(1,1,1,0.7);

        glBegin( GL_POINTS );
        for ( int i=0; i<nFrames; i++) {
            float l = ch0[i];
            glVertex3f( i*3.0f, l*100.0f - 200.0f, 0);
        }
        glEnd();

        glBegin( GL_POINTS );
        for ( int i=0; i<nFrames; i++) {
            float l = ch1[i];
            glVertex3f( i*3.0f, l*100.0f + 200.0f, 0);
        }
        glEnd();

    } glPopMatrix();
    
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
