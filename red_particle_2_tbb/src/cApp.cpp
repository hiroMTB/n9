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
        mPln.push_back( Perlin() );
        mPln[i].setSeed( 13251+i*123 );
        mPln[i].setOctaves(4);

        mPln2.push_back( Perlin() );
        mPln2[i].setSeed( 131+i*91 );
        mPln2[i].setOctaves(4);
    }
    
    int cnt = 0;
    for (int y=0; y<mH; y+=4) {
        for (int x=0; x<mW; x+=4) {
            int tg = cnt %4;

            Vec2f vel = mPln[tg].dfBm(x*0.005, y*0.005 );
            vbo.addPos( Vec3f(randFloat()*mW*1.1, randFloat()*mH*1.1, 0) );
            vbo.addCol( ColorAf(1,0,0,1) );
            spd.push_back( Vec3f(vel.x, vel.y, 0) );
            dest.push_back( Vec3f( absPos[tg].x/2, absPos[tg].y/2, 0) );
            cnt++;
        }
    }
    vbo.init( GL_POINTS, true, true );

#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){

    int loadNum = vbo.getPos().size();

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
    
    
    // hide
    ColorAf c = vbo.getCol()[i];

    if( c.a != 0){

        if( frame>200 ){
            int nKillParticle = vbo.getPos().size() * 0.0002;
            int kId  = (frame-200)*nKillParticle;
            
            if( i<kId ){
                c.a = 0;
                vbo.writeCol(i, c);
                return;
            }
        }

        int tg = (int)i%4;
        float resRate = 0.3;
        int life = frame - (int)i/400;
        
        Vec3f p = vbo.getPos()[i];
        Vec3f n = mPln[0].dfBm( p.x*0.001f*resRate, p.y*0.001f*resRate, frame*0.01f ) * 0.2f;
        
        if( frame < 200 ){
            spd[i].x += n.x*n.z;
            spd[i].y += n.y*n.z;
            spd[i].z = 0;
        }else{
            Vec3f dir = dest[i] - p;
            Vec3f n2 = mPln2[0].dfBm( p.x*0.001f*resRate, p.y*0.001f*resRate, 1 ) * 0.2f;
            spd[i].x += n2.x;
            spd[i].y += n2.y;
            spd[i].z = 0;
            
            float len = spd[i].length();

            len = MIN(len, dir.length()*0.5 );

            spd[i] = spd[i].normalized()*0.99 + dir*0.01;
            spd[i] *= len;
        }
        
        int substep = 10;
        float rate = 1.0f / substep;
        Vec3f adder = spd[i] * rate;
        
        for( int s=0; s<substep; s++){
            p += adder;
            Vec3f dir = dest[i] - p;
            double dist = dir.length();
            if( dist <= 2.5 ){
                ColorAf c = vbo.getCol()[i];
                c.a = 0;
                vbo.writeCol(i, c);
                break;
            }
        }
        
        vbo.writePos(i, p);
    }
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
