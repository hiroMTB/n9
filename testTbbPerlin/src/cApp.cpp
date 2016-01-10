#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Perlin.h"
#include "cinder/Rand.h"

#include "tbb/tbb.h"

#include "Exporter.h"
#include "mtUtil.h"
#include "VboSet.h"

//#define RENDER

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace tbb;

class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    void task(float i, float frame);
    
    void keyDown( KeyEvent event );
    void mouseMove( MouseEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void resize();
    
    Exporter mExp;
    const int mW = 1080*4/4;
    const int mH = 1920/4;
    
    VboSet vbo;
    vector<Vec3f> spd;
    vector<Vec3f> dest;

    Perlin mPln;
    int totalSize = mW * mH;
    Vec3f center = Vec3f(mW/2, mH/2, 0);
};

void cApp::setup(){
    setWindowPos( 0, 0 );
    setWindowSize( mW*2, mH*2 );
    mPln.setSeed(13251);
    mPln.setOctaves(4);

    for (int i=0; i<2; i++) {
            for (int y=0; y<mH; y++) {
                for (int x=0; x<mW; x++) {
                    Vec3f vel = mPln.dfBm(x*0.005, y*0.005, i*0.01)*2;
                    vbo.addPos( Vec3f(x, y, 0) );
                    vbo.addCol( ColorAf(1,0,0,0.5) );
                    spd.push_back( Vec3f(vel.x, vel.y+vel.z*1.5, 0) );
                    
                    dest.push_back( Vec3f(x, y, 0) );
                }
        }
    }
    
    vbo.init(true, true, true, GL_POINTS);
    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::task( float i, float frame ){

    Vec3f p = vbo.getPos()[i];
    Vec3f n = mPln.dfBm( p.x*0.005f, p.y*0.005f, frame*0.1f );
    n.x = n.x * n.z;
    n.y = n.y * n.z;

    int life = getElapsedFrames() - (int)i/10000;

    if( life<=400){
        spd[i].x += pow(n.x, 3)*3.0 * n.z;
        spd[i].y += pow(n.y, 3)*3.0 * n.z;

    }else{
        float rate = 1.0f/ (1+(life-400)*0.01);
        rate = MAX(rate, 0.1);
        spd[i].x += (n.x * rate);
        spd[i].y += (n.y * rate);
//        spd[i].z += (n.z * rate);

    }
    
    p.x += spd[i].x;
    p.y += spd[i].y;
    // p.z += spd[i].z;
    
    vbo.writePos(i, p);
    
    if( life > 400 ){
        
        Vec3f dir = dest[i] - p;
        dir = dir.normalized();
        
        float len = spd[i].length();
        spd[i] = spd[i].normalized()*0.9 + dir*0.1;
        spd[i] *= len;
    }else{
    
    }
}

void cApp::update(){

    float frame = getElapsedFrames();
    int loadNum = frame/400.0f * vbo.getPos().size();
    loadNum = MIN( vbo.getPos().size(), loadNum );
    
    parallel_for( blocked_range<size_t>(0, loadNum), [=](const blocked_range<size_t>& r) {
        for( size_t i=r.begin(); i!=r.end(); ++i ){
            task( i, frame );
        }
    });
    
    vbo.updateVboPos();
}

void cApp::draw(){

    gl::enableAlphaBlending();
    
    glPointSize(1);
    gl::clear( Colorf(0,0,0) );
    glPushMatrix();
    glScalef(2, 2, 1);
    vbo.draw();
    glPopMatrix();
    
    gl::color(0, 0, 0);
    gl::drawString( "fps : "+to_string(getAverageFps()) , Vec2i(20,20));
    gl::drawString( "frame : "+to_string(getElapsedFrames()) , Vec2i(20,40));

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
