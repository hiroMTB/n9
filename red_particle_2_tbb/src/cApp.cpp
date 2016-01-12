//#define RENDER
#define PROXY

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Perlin.h"
#include "cinder/Rand.h"

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
    void mouseMove( MouseEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void resize();
    
    Exporter mExp;
    int mW;
    int mH;
    
    VboSet vbo;
    vector<Vec3f> spd;
    vector<Vec3f> dest;

    Perlin mPln;
    Perlin mPln2;

    int totalSize = mW * mH;
    Vec3f center = Vec3f(mW/2, mH/2, 0);
    int frame = 0;
    
    vector<Vec2i> absPos = { Vec2i(1640,192), Vec2i(2680,192), Vec2i(1640,1728), Vec2i(2680,1728) };
};

void cApp::setup(){

#ifdef PROXY
    mW = 1000;
    mH = 1000;
#else
    mW = 1080*4;
    mH = 1920;
#endif
    setWindowSize( mW, mH );
    mExp.setup( mW, mH, 0, 1000-1, GL_RGB, mt::getRenderPath(), 0);
    setWindowPos( 0, 0 );
    mPln.setSeed(13251);
    mPln.setOctaves(4);

    mPln2.setSeed(29131);
    mPln2.setOctaves(4);

    for (int y=0; y<mH; y+=2) {
        for (int x=0; x<mW; x+=2) {
            Vec2f vel = mPln.dfBm(x*0.005, y*0.005 )*2;
            vbo.addPos( Vec3f(randFloat()*mW, randFloat()*mH, 0) );
            //vbo.addPos( Vec3f(x, y, 0) );
            vbo.addCol( ColorAf(1,0,0,1) );
            spd.push_back( Vec3f(vel.x, vel.y, 0) );
            dest.push_back( Vec3f(200, 200, 0) );
        }
    }
    vbo.init( GL_POINTS, true, true );

#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){

    int loadNum = vbo.getPos().size();
                  
    parallel_for( blocked_range<size_t>(0, loadNum), [=](const blocked_range<size_t>& r) {
        for( size_t i=r.begin(); i!=r.end(); ++i ){
            task( i );
        }
    });
    
    vbo.updateVboPos();
}


void cApp::task( float i  ){

    //int life = getElapsedFrames() - (int)i/300;
    Vec3f p = vbo.getPos()[i];
    Vec3f n = mPln.dfBm( p.x*0.001f, p.y*0.001f, (frame)*0.01f ) * 0.2f;
    
    spd[i].x += n.x*n.z;
    spd[i].y += n.y*n.z;
    
    p.x += spd[i].x;
    p.y += spd[i].y;
    
    if( frame > 200 ){
        Vec3f dir = dest[i] - p;
        double dist = dir.length();
        
        dir.normalize();
        Vec3f n2 = mPln2.dfBm( p.x*0.0001, p.y*0.0001 ,frame*0.01 )*0.1;
        dir += n2;
        dir.normalize();
        
        float len = spd[i].length();
        len = MAX(0.5, len);
        spd[i] = spd[i].normalized()*0.95 + dir*0.05;
        spd[i] *= len;
        
        if( dist < 6 ){
            ColorAf c = vbo.getCol()[i];
            c.a = 0;
            vbo.writeCol(i, c);
        }
            
    }
    
    vbo.writePos(i, p);

}

void cApp::draw(){

    gl::enableAlphaBlending();
    
    mExp.beginOrtho(); {
    glPointSize(1);
    gl::clear( Colorf(0,0,0) );
    glPushMatrix();
    vbo.draw();

    glTranslatef( 0.1, 0, 0 );
    vbo.draw();
    glPopMatrix();

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
