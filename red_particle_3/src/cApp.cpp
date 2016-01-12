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
    bool bOrtho = true;
    
    VboSet vbo;
    vector<Vec3f> spd;
    vector<Vec3f> dest;

    vector<Perlin> mPln;
    vector<Perlin> mPln2;

    int totalSize = mW * mH;
    Vec3f center = Vec3f(mW/2, mH/2, 0);
    int frame = 0;
    int substep = 6;
    
    vector<Vec2i> absPos = { Vec2i(1640,192), Vec2i(2680,192), Vec2i(1640,1728), Vec2i(2680,1728) };
};

void cApp::setup(){

#ifdef PROXY
    mW = 1080*4/2;
    mH = 1920/2;
#else
    mW = 1080*4;
    mH = 1920;
#endif
    setWindowSize( mW, mH );
    mExp.setup( mW, mH, 0, 1000-1, GL_RGB, mt::getRenderPath(), 0);
    setWindowPos( 0, 0 );
    
    for( int i=0; i<4; i++){
        mPln.push_back( Perlin(4, 13251+i) );
        mPln2.push_back( Perlin(4, 2131+i) );
    }
    
    int cnt = 0;
    for (int y=0; y<mH; y+=1) {
        for (int x=0; x<mW; x+=1) {
            int tg = cnt %4;

            float n =  mPln[0].noise(cnt*0.000001) * 3.1415*3.0;
            float r =  abs(mPln[0].noise(x*0.2+0.1, y+0.1+0.2))*2.0 + 3;
            
            Vec3f v = Vec3f( 3, 0, 0);
            v.rotateZ( n );
            
            vbo.addPos( Vec3f( absPos[tg].x/2, absPos[tg].y/2, 0) );
            vbo.addCol( ColorAf(1,0,0,1) );
            spd.push_back( v );
            dest.push_back( Vec3f( x, y, 0) );
            cnt++;
        }
    }
    vbo.init( GL_POINTS, true, true );

#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){

    int loadNum = frame * 3000*2;
    loadNum = MIN( loadNum, spd.size() );

   // for( int s=0; s<substep; s++){
        parallel_for( blocked_range<size_t>(0, loadNum), [=](const blocked_range<size_t>& r) {
            for( size_t i=r.begin(); i!=r.end(); ++i ){
                task( i );
            }
        });
    //}
    
    vbo.updateVboPos();
    vbo.updateVboCol();
}


void cApp::task( float i  ){
    
    int tg = (int)i%4;
    float resRate = 1;
    int life = frame - (int)i/400;
    
    Vec3f p = vbo.getPos()[i];
    
    Vec3f n = mPln[tg].dfBm( p.x*0.001f*resRate, p.y*0.001f*resRate, frame*0.001f )*0.75;
    spd[i].x += n.x;
    spd[i].y += n.y;
    
    if( frame < 200 ){
        float r = abs( mPln[0].fBm( frame*0.01, i*0.01f ) );
        float ang = mPln[1].fBm( frame*0.01,  i*0.01f ) * 3.1415f*2.0f;
        
        Vec3f n2;
        n2.x = cos(ang) * r;
        n2.y = sin(ang) * r;
        n2.z = 0;
        spd[i] += n2*0.1;
    }else{
        Vec3f dir = dest[i] - p;
        float speed = spd[i].length();
        spd[i] = spd[i].normalized()*0.999 + dir.normalized()*0.001;
        spd[i] *= speed;
    }
    
    p += spd[i];
    vbo.writePos(i, p);
}

void cApp::draw(){
    
    bOrtho ? mExp.beginOrtho() : mExp.beginPersp(); {

    gl::enableAlphaBlending();

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
        case 'o':   bOrtho = !bOrtho;         break;
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
