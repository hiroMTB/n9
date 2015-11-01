#define RENDER

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Perlin.h"

#include "Exporter.h"
#include "mtUtil.h"
#include "StrangeAgent.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Rand.h"

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

    const int mW = 4320; // 1080*4
    const int mH = 1920;
    const float mScale = 1;

    Exporter mExp;
    Perlin mPln;
    MayaCamUI mCamUi;
    CameraOrtho ortho;
    
    vector<StrangeAgent> mSAs;
    vector<Vec3f> mPlnPts;
    
    int total;
    float scale = 0.000007f;

};

void cApp::setup(){
    
    setWindowPos( 0, 0 );
    setWindowSize( mW*0.5, mH*0.5 );
    mExp.setup( mW*mScale, mH*mScale, 2999, GL_RGB, mt::getRenderPath(), 0, true);
    
    mPln.setOctaves(4);
    mPln.setSeed(1332);

    randSeed( mt::getSeed() );
    
    int count = 0;
    for( int i=0; i<100; i++){
        StrangeAgent sa;
        sa.setRandom();
        mSAs.push_back( sa );

        for(int j=0; j<sa.points.size(); j++){
            mPlnPts.push_back( Vec3f(count*scale,0,0) );
            count++;
        }
    }
    
    total = count;
    
    if( 1 ){
        CameraPersp cam;
        cam.setNearClip(0.1);
        cam.setFarClip(1000000);
        cam.setFov(60);
        cam.setEyePoint( Vec3f(0,0,-30 ) );
        cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
        cam.setAspectRatio( (float)mW/mH );
        mCamUi.setCurrentCam(cam);
    }else{
        ortho.setNearClip(0.1);
        ortho.setFarClip(1000000);
        ortho.setEyePoint( Vec3f(0,0,-7 ) );
        ortho.setCenterOfInterestPoint( Vec3f(0,0,0) );
        ortho.setAspectRatio( (float)mW/mH );
    }
    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){
    
    for( int j=0; j<256*4; j++ ){
        for( int i=0; i<mSAs.size(); i++ ){
            mSAs[i].update();
        }
    }
    
    int count = 0;
    for( int i=0; i<mSAs.size(); i++ ){
        for( int j=0; j<mSAs[i].points.size(); j++ ){
            Vec3f p = mSAs[i].points[j];
//            Vec3f n = mPln.dfBm( p );
//            n *= 0.02f;
            mPlnPts[count] += p*0.1;
            
            count++;
        }
    }
    
}

void cApp::draw(){

    mExp.begin( mCamUi.getCamera() );
    {
        gl::clear( Colorf(0,0,0) );
    
        if(0){
            glPushMatrix();
            glTranslatef( 5, 0, 0);
            gl::color(1, 1, 1);
            glBegin( GL_POINTS );
            for( int i=0; i<mSAs.size(); i++ ){
                for (int j=0; j<mSAs[i].points.size(); j++) {
                    glVertex3f( mSAs[i].points[j] );
                }
            }
            glEnd();
            glPopMatrix();
        }
        
        glPushMatrix();
        glTranslatef( -total*scale*0.5, 0, 0);
        gl::color(0, 0, 1, 0.35);
        glBegin( GL_POINTS );
        for( int i=0; i<mPlnPts.size(); i++ ){
            glVertex3f( mPlnPts[i] );
        }
        glEnd();
        glPopMatrix();
        
        glPushMatrix();
        glColor3f(1, 0, 0);
        glBegin( GL_LINES );
        glVertex3f(-100, 0, 0);
        glVertex3f(100, 0, 0);
        glEnd();
        glPopMatrix();
    }
    mExp.end();
    
    gl::clear( Colorf(1,1,1) );
    gl::color( Colorf(1,1,1) );
    mExp.draw();
}

void cApp::keyDown( KeyEvent event ){

    for( int i=0; i<mPlnPts.size(); i++ ){
        mPlnPts[i] = Vec3f(i*scale,0,0);
    }
    
    for( int i=0; i<mSAs.size(); i++ ){
        mSAs[i].setRandom();
    }
    
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
