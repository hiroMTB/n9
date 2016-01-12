#define RENDER
//#define PROXY

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Perlin.h"
#include "cinder/Rand.h"
#include "cinder/MayaCamUI.h"

#include "tbb/tbb.h"

#include "Exporter.h"
#include "mtUtil.h"
#include "VboSet.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace tbb;

class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    void task(float i );
    
    void keyDown( KeyEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    
    Exporter mExp;
    int mW;
    int mH;
    bool bOrtho = false;
    bool bShowNoise = false;
    
    VboSet vbo;
    vector<Vec3f> spd;
    vector<Vec3f> dest;
    vector<Colorf> noise;


    vector<Perlin> mPln;

    int totalSize = mW * mH;
    double frame = 0;
    
    vector<Vec2i> absPos = { Vec2i(1640,192), Vec2i(2680,192), Vec2i(1640,1728), Vec2i(2680,1728) };
    
    MayaCamUI camUi;
    
    Vec2f noiseCenter;
};

void cApp::setup(){

#ifdef PROXY
    mW = 1080*4/2;
    mH = 1920/2;
    setWindowSize( mW, mH );
#else
    mW = 1080*4;
    mH = 1920;
    setWindowSize( mW/2, mH/2 );
#endif
    setWindowPos( 0, 0 );
    mExp.setup( mW, mH, 0, 1000-1, GL_RGB, mt::getRenderPath(), 0);
    
    mPln.push_back( Perlin(4, 132051) );

    noiseCenter.x = mW * 0.8;
    noiseCenter.y = mH * 0.45;
    
    int cnt = 0;
    for (int y=0; y<mH; y+=6) {
        for (int x=0; x<mW; x+=6) {

            Vec3d p = mPln[0].dfBm(x*0.0001+randFloat(), y*0.0001+randFloat(), cnt*0.01);
            p.x += 1.0;
            p.y += 1.0;
            p.x *= 0.5;
            p.y *= 0.5;

            p.x *= mW;
            p.y *= mH;

            p.x += noiseCenter.x/2;
            p.y += noiseCenter.y/2;
            p.z = 0;
            vbo.addPos( p + Vec3f( randFloat(), randFloat(), 0)*100.0 );
            vbo.addCol( ColorAf(1,0,0,1) );

            Vec2d vel = mPln[0].dfBm(x*0.005, y*0.005 );
            spd.push_back( Vec3f(vel.x, vel.y, 0) );

            cnt++;
            
            noise.push_back(Colorf(0,0,1));
        }
    }
    vbo.init( GL_POINTS, true, true );

    CameraPersp cam(mW, mH, 60, 1, 10000);
    camUi.setCurrentCam( cam );
    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){

    int loadNum = vbo.getPos().size() * MIN(frame/100.0, 1.0);

    if( 1 ){
        parallel_for( blocked_range<size_t>(0, loadNum), [=](const blocked_range<size_t>& r) {
            for( size_t i=r.begin(); i!=r.end(); ++i ){
                task( i );
            }
        });
    }else{
        for( int s=0; s<loadNum; s++){
            task( s );
        }
    }
    
    vbo.updateVboPos();
    vbo.updateVboCol();
}


void cApp::task( float i  ){
    
    double ox = 6;
    double oy = 4.2;
    
    double resRate = 0.2;
    
    Vec3f p = vbo.getPos()[i];
    Vec3f n = mPln[0].dfBm( ox+p.x*0.001*resRate, oy+p.y*0.001*resRate, 2.1+frame*0.00001 );

    noise[i] = Colorf( n.x*0.5+1, n.y*0.5+1, n.z*0.5+1 );
    
    double rate = (frame+1)/300.0 + 0.2;
    if(rate>1){
        rate = rate*rate*rate*rate*rate;
        rate = MIN(rate, 1000);
    }
    
    float oRate = MIN(rate, 1.0);

    spd[i].x = spd[i].x * (1.0-oRate) + n.x*rate;
    spd[i].y = spd[i].y * (1.0-oRate) + n.y*rate;
//    spd[i].z = spd[i].z * (1.0-oRate) + n.z*rate;
    
    p.x += spd[i].x;
    p.y += spd[i].y;
//    p.z += spd[i].z;
    
    vbo.writePos(i, p);

}

void cApp::draw(){
    
    bOrtho ? mExp.beginOrtho() : mExp.beginPersp(60, 0.1, 100000); {
    //bOrtho ? mExp.beginOrtho() : mExp.begin( camUi.getCamera() ); {

        gl::enableAlphaBlending();
        
        glPointSize(1);
        gl::clear( Colorf(0,0,0) );
        gl::pushMatrices();
        
        glTranslatef( mW/2-noiseCenter.x, mH/2-noiseCenter.y, 0);
        vbo.draw();
        gl::popMatrices();

    } mExp.end();
    
    mExp.draw();
    
    gl::color(0, 0, 0);
    gl::drawString( "fps : "+to_string(getAverageFps()) , Vec2i(20,20));
    gl::drawString( "frame : "+to_string(getElapsedFrames()) , Vec2i(20,40));
    
    frame++;
}

void cApp::keyDown( KeyEvent event ){
    switch (event.getChar()) {
        case 's':   mExp.snapShot();          break;
        case 'S':   mExp.startRender();       break;
        case 'T':   mExp.stopRender();        break;
        case 'f':   frame+=100;               break;
        case 'o':   bOrtho = !bOrtho;         break;
        case 'n':   bShowNoise = !bShowNoise; break;

    }
}

void cApp::mouseDown( MouseEvent event ){
    camUi.mouseDown( event.getPos() );
}

void cApp::mouseDrag( MouseEvent event ){
    camUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
