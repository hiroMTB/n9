//#define RENDER

/*
 * 
 *  nested particle emitter
 *
 */

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

Perlin gPln = Perlin(4, 123);
int frame = 0;


class Particle;
typedef std::shared_ptr<Particle> ParticleRef;

class Particle{
    
public:

    static int id_cnt;
    
    Particle(){
        id = id_cnt++;
        bdue = randInt(50, 1000);
    }
    
    int id;
    int age = 0;
    int bdue = 5;   // age of making children
    Vec3f pos;
    Vec3f vel;
    Vec3f acc;
    ColorAf col;
    int nestLevel = 0;  // 0 = mother
    ParticleRef mother;
    vector<ParticleRef> chirdlen;
    
    void update(){
        age++;
        pos += vel;
        Vec3f dir = vel.normalized();
        dir.rotate(gPln.dfBm( 2, 1+id, frame ), randFloat(-3.0f, 3.0f));
        Vec3f n = dir * 0.01;
        vel.x += n.x;
        vel.y += n.y;
        // we shold use interesting data set here, photo or etc
        
        pos.z = 0;
        col.a -= 0.0001;
    }
    
    static ParticleRef create(){ return make_shared<Particle>(); };
    
    gl::TextureRef tex;
};

int Particle::id_cnt = 0;

class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    void create_at_start();
    void keyDown( KeyEvent event );
    void mouseMove( MouseEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    
    int mW = 2560;
    int mH = 1600;
    bool bOrtho = true;

    vector<Vec3f> absPos = { Vec3f(1640,192,0), Vec3f(2680,192,0), Vec3f(1640,1728,0), Vec3f(2680,1728,0) };
    
    Exporter mExp;
    VboSet vPoint;
    VboSet vLine;
    
    vector<ParticleRef> ptcls;

    
    Vec3f startPos = Vec3f(mW/6, mH/2, 0);
};

void cApp::create_at_start(){
    ParticleRef p = Particle::create();
    p->pos = startPos;
    p->vel = randVec3f();
    p->vel.z = 0;
    p->nestLevel = 0;
    p->col = ColorAf(1,0,0,0.9);
    ptcls.push_back(p);
}


void cApp::setup(){

    setWindowSize( mW, mH );
    setWindowPos( 0, 0 );

    mExp.setup( mW, mH, 0, 1000-1, GL_RGB, mt::getRenderPath(), 0);
    
    for( int i=0; i<3; i++){
        create_at_start();
    }

#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){

    create_at_start();

    for( int i=0; i<ptcls.size(); i++){
        ptcls[i]->update();        
    }

    vector<ParticleRef> newChildren;
    
    for( int i=0; i<ptcls.size(); i++){
        ParticleRef m = ptcls[i];
        
        if( m->age==m->bdue ){
            float rate = abs(gPln.noise(m->id*0.2, frame*0.2))+ 0.33f;
            int nChild = rate * 3.2f * randFloat(10, 100);
            
            if(!m->mother) nChild = 10;
            
            for( int j=0; j<nChild; j++){
                ParticleRef p = Particle::create();
                p->pos = m->pos;
                Vec3f rot(1,1,1);
                rot.rotate(Vec3f(0,0,1), gPln.fBm(m->id*2, randFloat()*3.0f ) );
                p->vel = m->vel + rot;
                p->col = m->col;
                p->col.a -= 0.001f;
                p->nestLevel = m->nestLevel+1;
                p->mother = m;
                newChildren.push_back(p);
                
                m->chirdlen.push_back(p);
            }
            m->col.a = 0;
        }
    }
    
    ptcls.insert(ptcls.end(), newChildren.begin(), newChildren.end());
    
    
    // VBO point
    vPoint.resetVbo();
    for( int i=0; i<ptcls.size(); i++){
        ParticleRef p = ptcls[i];
        vPoint.addPos(p->pos);
        vPoint.addCol(p->col);
    }

    vPoint.init(GL_POINTS);

    // VBO line
    
    if(1){
        vLine.resetVbo();
        for( int i=0; i<ptcls.size(); i++){
            
            ParticleRef p = ptcls[i];
            
            float limitAge = abs(gPln.noise(p->age*0.1, p->id*0.1, frame*0.1)) * 200;
            if(p && p->age<limitAge){
                
                ParticleRef m = p->mother;
                
                if(m){
                    vLine.addPos(p->pos);
                    vLine.addPos(m->pos);
                }else{
                    vLine.addPos(p->pos);
                    vLine.addPos(startPos);
                }
                
                ColorAf c(1,0,0,0.1);
                vLine.addCol(c);
                vLine.addCol(c);
            }
        }
        
        vLine.init(GL_LINES);
    }
    
//    int loadNum = 0;
//    parallel_for( blocked_range<size_t>(0, loadNum), [=](const blocked_range<size_t>& r) {
//        for( size_t i=r.begin(); i!=r.end(); ++i ){
//        }
//    });

    
    
}



void cApp::draw(){

    glPointSize(1);
    glLineWidth(1);
    
    bOrtho ? mExp.beginOrtho() : mExp.beginPersp(); {

        gl::enableAlphaBlending();
        
        gl::clear( Colorf(0,0,0) );
        glPushMatrix();
        
        vLine.draw();
        vPoint.draw();
        
        gl::color(1, 0, 0);
        gl::drawStrokedCircle( Vec2i(startPos.x, startPos.y), 5);
        
        glPopMatrix();
        
    } mExp.end();
    
    mExp.draw();
    
    gl::color(0, 0, 0);
    gl::drawString( "fps : "+to_string(getAverageFps()) , Vec2i(20,20));
    gl::drawString( "frame : "+to_string(getElapsedFrames()) , Vec2i(20,40));
    gl::drawString( "numParticles : "+to_string(ptcls.size()) , Vec2i(20,60));
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

CINDER_APP_NATIVE( cApp, RendererGl(0) )
