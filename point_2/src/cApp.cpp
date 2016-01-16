#define RENDER
#include "cinder/app/AppNative.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "cinder/Perlin.h"

#include "mtUtil.h"
#include "Exporter.h"
#include "TbbNpFinder.h"
#include "mtSvg.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
public:
    void setup();
    void update();
    void draw();
    void keyDown( KeyEvent event );
    
    const int win_w = 1920;
    const int win_h = 1920;
    int frame = 0;
    
    vector<vector<Colorf>> mColorSample1;
    vector<vector<Colorf>> mColorSample2;
    
    Exporter mExp;
    fs::path assetDir;
    
    vector<Vec2f> ps;
    vector<Vec2f> vs;
    vector<ColorAf> cs;
    vector<BSpline2f> mBSps;
    vector<Perlin> mPlns{ Perlin(4, 111), Perlin(4, 512), Perlin(4, 773), Perlin(4, 980) };
    mtSvg guide;

};

void cApp::setup(){
    
    setFrameRate( 25 );
    setWindowSize( win_w*0.5, win_h*0.5 );
    setWindowPos( 0, 0 );
    mExp.setup( win_w, win_h, 0, 3000, GL_RGB, mt::getRenderPath(), 0);
    
    assetDir = mt::getAssetPath();
    mt::loadColorSample("img/colorSample/n5pickCol_2.png", mColorSample1 );
    mt::loadColorSample("img/colorSample/n5pickCol_3.png", mColorSample2 );
    
    guide.load( assetDir/"svg"/"n5_guide.svg" );
    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){

    
    /*
        add particle
    */
    
    int numPs = ps.size();
    
    if( frame%100 != 0 ){
        
        int num = randInt(10, 30);
        
        for( int i=0; i<num; i++ ){
            
            // pick up color
            int cid = numPs + i;
            int cidx = cid % mColorSample1.size();
            int cidy = cid / mColorSample1.size();
            cidx %= mColorSample1.size();
            cidy %= mColorSample1[0].size();
            ColorAf col = mColorSample1[cidx][cidy];
            float angle = mPlns[1].fBm( cid*0.01 ) * 10.0;
            Vec2f vel = Vec2f(1,1) * randFloat(10, 20);
            vel.rotate(angle);
            
            ps.push_back( Vec2f(0,0) );
            cs.push_back( col );
            vs.push_back( vel );
        }
        
    }else{
        int num = randInt(20000, 30000);
    
        for( int i=0; i<num; i++ ){
            
            // pick up color
            int cid = numPs + i;
            int cidx = cid % mColorSample1.size();
            int cidy = cid / mColorSample1.size();
            cidx %= mColorSample1.size();
            cidy %= mColorSample1[0].size();
            ColorAf col = mColorSample1[cidx][cidy];
            
            Vec3f n = mPlns[0].dfBm( frame, col.r*col.g, col.b );
            Vec2f vel;
            vel.x = n.x * n.y * 20;
            vel.y = n.y * n.z * 20;
            
            if(vel.length() <= 20){
                vel = vel.normalized() * lmap(abs(n.z), 0.0f, 1.0f, 0.7f, 1.0f) *20;
            }
            
            ps.push_back( Vec2f(0,0) );
            cs.push_back( col );
            vs.push_back( vel );
        }
    }
    
    
    
    /*
        update particle
    */
    for( int i=0; i<ps.size(); i++ ){
        Vec2f & p = ps[i];
        Vec2f & v = vs[i];
        ColorAf & c = cs[i];
        
        
        Vec3f n = mPlns[1].dfBm( frame*0.01, i*0.01, c.b*0.5 );
        c.r += n.x * 0.02;
        c.g += n.y * 0.02;
        c.b += n.z * 0.02;
        
        Vec2f new_vel;
        new_vel.x = n.x;
        new_vel.y = n.y;
        v += new_vel*2;
        
        p += v;
        
        v *= 0.999;
        c.a *= 0.999;
        
    }
    
    
    /*
        
        remove
     
     */
    vector<Vec2f>::iterator pit = ps.begin();
    vector<Vec2f>::iterator vit = vs.begin();
    vector<ColorAf>::iterator cit = cs.begin();
    
    int xmin = -mExp.mFbo.getWidth()/2;
    int xmax = mExp.mFbo.getWidth()/2;
    int ymin = -mExp.mFbo.getHeight()/2;
    int ymax = mExp.mFbo.getHeight()/2;
    
    while ( pit!=ps.end() ) {
        Vec2f & p = *pit;
    
        if( (p.x<xmin || xmax<p.x) || p.y<ymin || ymax<p.y  ){
            pit = ps.erase(pit);
            vit = vs.erase(vit);
            cit = cs.erase(cit);
        }else{
            pit++;
            vit++;
            cit++;
        }
    }
    
    frame++;
}

void cApp::draw(){
    mExp.beginOrtho();{
        gl::clear();
        gl::enableAlphaBlending();
        glPushMatrix();
        gl::translate( mExp.mFbo.getWidth()/2, mExp.mFbo.getHeight()/2 );
        
        if( mExp.mFrame==1 || (!mExp.bRender && !mExp.bSnap) ){
            //guide.draw();
            mt::drawScreenGuide();
        }
        
        /*
            draw particle
        */
        glBegin( GL_POINTS );
        glPointSize(1);
        for( int i=0; i<ps.size(); i++ ){
            Vec2f & p = ps[i];
            ColorAf & c = cs[i];
            //ColorAf c( 0.7, 0.7, 0.7, 0.5 );
            glColor4f( c );
            
            glVertex2f( p );
        }
        glPopMatrix();
        glEnd();
        
        
    }mExp.end();

    gl::clear( ColorA(0,0,0,1) );
    gl::color( Color::white() );
    mExp.draw();

    gl::drawString("Frame: " + to_string(frame), Vec2f(50, 50) );
    
#ifdef RENDER
    frame+=1;
#else
    frame+=5;
#endif

}

void cApp::keyDown( KeyEvent event ){

    switch (event.getChar()) {
        
        case 'R':
            mExp.startRender();
            break;

        case 'T':
            mExp.stopRender();
            break;

        case 'f':
            frame+=100;
            break;
            
        default:
            break;
    }
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
