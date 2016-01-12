//#define RENDER
#define PROXY

#include "cinder/app/AppNative.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Perlin.h"

#include "mtUtil.h"
#include "Exporter.h"
#include "VboSet.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Particle{

public:
    Vec2f posf;
    Vec2f pos;
    Vec2f vel;
    
    Colorf col;
    Vec3f hsv;
    float key;
    uint type;
    bool hide = false;
    bool start = false;
    bool operator<(const Particle& p) const{ return key < p.key; }
    bool operator>(const Particle& p) const{ return key > p.key; }
    
    int frame = 0;
};


class cApp : public AppNative {
public:
    void setup();
    void update();
    void draw();
    void update_vbo();
    void update_particles();

#ifndef RENDER
    void keyDown( KeyEvent event );
#endif
    
    int win_w = 1080*4;
    int win_h = 1920;
    
    const float fps = 25.0f;
    int frame = 0;
    
    vector<Vec2f> target = { Vec2f(1640,192), Vec2f(2680,192), Vec2f(1640,1728),Vec2f(2680,1728) };
    
    Exporter mExp;
    Perlin mPln;
    
    fs::path assetDir;
  
    vector<VboSet> vbos;
    vector<vector<Particle>> pts;
};

void cApp::setup(){
    
#ifndef RENDER
    setFrameRate( fps );
#endif
    
#ifdef PROXY
    win_w *= 0.5;
    win_h *= 0.5;
#endif

    setWindowSize( win_w, win_h );
    mExp.setup( win_w, win_h, 0, 1000-1, GL_RGB, mt::getRenderPath(), 0);
    setWindowPos( 0, 0 );
    assetDir = mt::getAssetPath();
    
    Surface32f img = Surface32f( loadImage(assetDir/"img"/"geo"/"earth"/"earth_15_4320x1920.png") );

    vector<Particle> tmp;
    Surface32f::Iter itr = img.getIter();
    while (itr.line()) {
        while( itr.pixel() ){
            Particle p;
            p.pos = p.posf = itr.getPos();
            p.col = Colorf(itr.r(), itr.g(), itr.b());
            p.hsv = rgbToHSV( p.col );
            p.key = p.hsv.z;
            tmp.push_back( p );
        }
    }
    
    std::sort( tmp.begin(), tmp.end() );
    
    
    // make vbo
    vbos.assign( 4, VboSet() );
    pts.assign(4, vector<Particle>() );
    
    int nPts = tmp.size();
    nPts -= nPts % 4;
    int nPerSet = nPts/4;
    
    for (int i=0; i<nPts; i++){
        Particle & p = tmp[i];
        
        int setId = i/nPerSet;
        vbos[setId].addPos( Vec3f(p.pos.x,p.pos.y, 0 ) );

        Colorf color(1, 0, 0);

        if( 0 ){
            switch(setId){
                case 0: color.set(1, 0, 0); break;
                case 1: color.set(0, 1, 0); break;
                case 2: color.set(0, 0, 1); break;
                case 3: color.set(1, 1, 0); break;
            }
        }
        vbos[setId].addCol( color );
        
        p.type = setId;
        p.vel = mPln.dfBm(setId, i*0.2);
        pts[setId].push_back(p);
    }

    for( auto & vbo : vbos ){
        vbo.init(GL_POINTS, true, true  );
    }

    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){
    
    update_particles();
    update_vbo();
    frame++;
}

void cApp::update_particles(){
    
    for (int i=0; i<pts.size(); i++) {
        
        for (int j=0; j<pts[i].size(); j++) {
            
            if(j < frame*1000){
                
                Particle & p = pts[i][j];
                
                if( p.hide )
                    continue;
                
                // vel update
                Vec2f dir = (target[i] - p.pos);
                float dist = dir.length();
                if( dist < 7.0f ){
                    p.hide = true;
                    p.vel = Vec2f(0,0);
                    p.pos = target[i];
                }else{
                    dir *= 0.1;
                    p.vel = p.vel*0.99 + dir*0.01;
                    p.vel = p.vel.normalized() * dir.length()*0.1;
                    p.pos += p.vel;
                }
            }
        }
    }
}

void cApp::update_vbo(){
    
    for( int i=0; i<pts.size(); i++){
        for (int j=0; j<pts[i].size(); j++){
            Particle & p = pts[i][j];
            vbos[i].writePos( j, Vec3f(p.pos.x,p.pos.y, 0 ) );
            
            if( p.hide )
                vbos[i].writeCol( j, ColorAf(0,0,0,0) );
        }
    }
    
    for ( auto & vbo : vbos ){
        vbo.updateVboPos();
    }
}

void cApp::draw(){
    
    mExp.beginOrtho();{
        
        gl::clear();
        gl::enableAlphaBlending();
        glPointSize( 1 );

        for( auto &vbo: vbos ){
            vbo.draw();
        }

        glTranslated(0.1, 0.1, 0);
        for( auto &vbo: vbos ){
            vbo.draw();
        }

    }mExp.end();
    
    mExp.draw();
    
    gl::drawString("Frame: " + to_string(frame), Vec2f(30, 20) );
    frame++;
}

#ifndef RENDER
void cApp::keyDown( KeyEvent event ){

    switch (event.getChar()) {
        case 's':   mExp.snapShot();          break;
        case 'S':   mExp.startRender();       break;
        case 'T':   mExp.stopRender();        break;
        case 'f':   frame+=100;               break;
    }
}
#endif

CINDER_APP_NATIVE( cApp, RendererGl(0) )
