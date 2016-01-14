//#define RENDER

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
#include "RfExporterBin.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace tbb;

class Particle{

public:
    
    Vec3f pos;
    Vec3f vel;
    Vec3f force;

    float mass;
    int id;
    int life;
    
    bool operator<( const Particle & p ) const {
        return life < p.life;
    }

    bool operator>( const Particle & p ) const {
        return life > p.life;
    }

};


class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    void keyDown( KeyEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void add_particle();
    void update_particle();
    void update_vbo();
    
    bool bOrtho = true;
    bool bTbb   = true;
    const int mW = 1080*4;
    const int mH = 1920;
    int ptclCount = 0;
    float dispScale = 0.55;
    float frame    = 0;
    fs::path assetDir = mt::getAssetPath();

    vector<Perlin> mPln;
    vector<Vec2i> absPos = { Vec2i(1640,192), Vec2i(2680,192), Vec2i(1640,1728), Vec2i(2680,1728) };

    vector<Particle> ptcl;
    VboSet vbo;
    Exporter mExp;
    
};

void cApp::setup(){

    setWindowSize( mW*dispScale, mH*dispScale );
    printf( "win : %0.2f, %0.2f\n", mW*dispScale, mH*dispScale );

    setWindowPos( 0, 0 );
    mExp.setup( mW, mH, 0, 1000-1, GL_RGB, mt::getRenderPath(), 0);
    
    mPln.push_back( Perlin(4, 5555) );
    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){

    add_particle();
    update_particle();
    update_vbo();
}

void cApp::add_particle(){

    int nAdd = 3000 + 1000 * (mPln[0].fBm( 1+frame*0.01 )+1.0);
    for( int i=0; i<nAdd; i++){
        
        Particle pt;
        pt.life = randInt(300, 600);
        pt.id = ptclCount++;
        pt.mass = randFloat(0.5, 1.1);
        pt.pos.set(absPos[0].x, absPos[0].y, 0);
        
        Vec3f v = mPln[0].dfBm( pt.id,  randFloat(-1,i)*0.1, randFloat(-i,1)*0.1 ) * 3.0;
        Vec3f f = mPln[0].dfBm( pt.id, randFloat(-1,i)*0.1,  randFloat(-i,1)*0.1 ) * 3.0;
        
        pt.vel = v;
        pt.force = f;
        ptcl.push_back( pt );
    }
}

void cApp::update_particle(){
    
    int loadNum = ptcl.size();
    int substep = 1;
    
    for( int j=0; j<substep; j++){
        parallel_for( blocked_range<size_t>(0, loadNum), [=](const blocked_range<size_t>& r) {
            for( size_t i=r.begin(); i!=r.end(); ++i ){
                Particle & pt = ptcl[i];
                Vec3f & p = pt.pos;
                Vec3f & v = pt.vel;
                Vec3f & f = pt.force;
                
                Vec3f n = mPln[0].dfBm( p.x*0.01 + randFloat(-1,1)*0.0001, p.y*0.01+ randFloat(-1,1)*0.0001, frame*0.02 ) * 1.0;

                //n.x *= abs(n.z+1.0);
                //n.y *= abs(n.z+1.0);
                //f = f*0.5 + n*0.5;

                n.x = pow(n.x, 3) * 5 * abs(n.z+1.0);
                n.y = pow(n.y, 3) * 5 * abs(n.z+1.0);
                n.rotateZ( (i+frame)*0.00001 );
                
                f = f*0.5 + n*0.5;
                
                v = v*0.99 + f;
                p += v;
                p.z = 0;
                pt.life--;
            }
        });
    }
}

void cApp::update_vbo(){

    vbo.resetVbo();
    
    for (int i=0; i<ptcl.size(); i++) {
        if( ptcl[i].life > 0){
            vbo.addPos( ptcl[i].pos );
            vbo.addCol( ColorAf(1,0,0,1) );
        }
    }
    
    if(vbo.vbo){
        vbo.updateVboPos();
        vbo.updateVboCol();
    }else{
        vbo.init( GL_POINTS );
    }
}

void cApp::draw(){
    
    bOrtho ? mExp.beginOrtho() : mExp.beginPersp(58, 1, 100000); {

        gl::enableAlphaBlending();
        
        glPointSize(2);
        gl::clear( Colorf(0,0,0) );
        gl::pushMatrices();
        vbo.draw();

        gl::popMatrices();

    } mExp.end();
    
    mExp.draw();
    
    gl::pushMatrices();
    glTranslatef( mW/2, mH/2, 0);
    mt::drawCoordinate();
    mt::drawScreenGuide();
    gl::popMatrices();
    
    gl::color(0, 0, 0);
    gl::drawString( "fps : "+to_string(getAverageFps()) , Vec2i(20,20));
    gl::drawString( "frame : "+to_string(getElapsedFrames()) , Vec2i(20,40));
    gl::drawString( "nParticle : "+to_string(vbo.getPos().size()) , Vec2i(20,60));
    
    frame++;
}

void cApp::keyDown( KeyEvent event ){
    switch (event.getChar()) {
        case 's':   mExp.snapShot();          break;
        case 'S':   mExp.startRender();       break;
        case 'T':   mExp.stopRender();        break;
        case 'f':   frame+=100;               break;
        case 'o':   bOrtho = !bOrtho;         break;
        case 'p':   bTbb = !bTbb;         break;
    }
}

void cApp::mouseDown( MouseEvent event ){
    cout << event.getPos() << endl;
}

void cApp::mouseDrag( MouseEvent event ){
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
