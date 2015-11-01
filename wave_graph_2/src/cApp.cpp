//#define RENDER

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/rand.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"
#include "cinder/Perlin.h"
#include "cinder/BSpline.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "ufUtil.h"
#include "Wave.h"
#include "Exporter.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    
    void draw_dot_wave( const Wave & wave, int start, int n);
    void draw_line_wave( const Wave & wave, int start, int n);

    void draw_line( const Wave & wave, int start, int n);
    void draw_line_out( const Wave & wave, int start, int n);

    void draw_low_res( const Wave & wave, int start, int n);

    void draw_perlin_wave( const Wave & wave, int start, int n);
    void draw_angle( const Wave & wave, int start, int n);
    void draw_rect( const Wave & wave, int start, int n);
    void draw_connect( const Wave & wave, int start, int n);
    void draw_circle( const Wave & wave, int start, int n);
    void draw_circle_big( const Wave & wave, int start, int n);
    void draw_circle_dot( const Wave & wave, int start, int n);
    
    void draw_spline( const Wave & wave, int start, int n);
    void draw_cross( const Wave & wave, int start, int n);
    void draw_fan( const Wave & wave, int start, int n);
    void draw_log( const Wave & wave, int start, int n);
    
    void loadColorSample( string fileName, vector<vector<Colorf>>& col );
    
    const int win_w = 4320;
    const int win_h = 1920;

    Perlin mPln;
    vector<Wave> mWave;
    vector<vector<Colorf>> mColorSample1;

    Exporter mExp;
  
    int frame = -1;
    int mFpb = 1024*2;
    float camDist = -1800;
    float amp = 270;
    
    MayaCamUI mayaCam;
};

void cApp::setup(){
    
    setFrameRate(25);
    setWindowSize( win_w*0.5, win_h*0.5 );
    setWindowPos( 0, 0 );
    gl::enableVerticalSync();
    
    mPln.setOctaves(4);
    mPln.setSeed(444);
    
    mExp.setup( win_w, win_h, 1000, GL_RGB, uf::getRenderPath(), 0 );
    uf::loadColorSample("img/geo/Mx80_2_org.jpg", mColorSample1 );
    
    for( int w=0; w<8; w++ ){
        mWave.push_back( Wave() );
        mWave[w].create( "snd/samples/192k/3s1e_192k_" + toString(w+1) + ".wav");
        mWave[w].pos.assign( mFpb, Vec3f(0,0,0) );
        mWave[w].color.assign( mFpb, ColorAf(0.5,0.5,0.5,1) );
        
        for( int c=0; c<mWave[w].color.size(); c++ ){
            int idx = (int)mWave[w].pos[c].x % mColorSample1.size();
            int idy = (int)mWave[w].pos[c].y % mColorSample1[0].size();
            mWave[w].color[c] = mColorSample1[idx][idy];
        }
    }
    
    CameraPersp persp;
    persp.lookAt( Vec3f(0,0, camDist), Vec3f(0,0,0), Vec3f(0,-1,0) );
    persp.setAspectRatio( (double)win_w/win_h );
    persp.setNearClip(1);
    persp.setFarClip(100000);
    mayaCam.setCurrentCam( persp );
    
#ifdef RENDER
    mExp.startRender();
#endif

}

void cApp::update(){

    frame++;
    
    int nFrame = mWave[0].buf->getNumFrames();
    long audioPos = frame * mFpb;
    
    if( audioPos >= nFrame ){
        
        quit();
    }
    
    for( int w=0; w<mWave.size(); w++ ){
    
        const float * ch0 = mWave[w].buf->getChannel( 0 );
        
        for ( int i=0; i<mFpb; i++) {
            float l = ch0[audioPos + i];
            mWave[w].pos[i].x = i;
            mWave[w].pos[i].y = l;
        }
        
        // color
        for( int i=0; i<mWave[w].color.size(); i++ ){
            ColorAf & oldColor = mWave[w].color[i];
            Vec3f noise = mPln.dfBm( getElapsedFrames()*0.1, mWave[w].pos[i].x*0.1, mWave[w].pos[i].y );
            noise *= 0.1;
            oldColor.r += noise.x*0.02;
            oldColor.g += noise.z*0.02;
            oldColor.b += noise.y*0.02;
            oldColor.a = 0.7;
        }
    }
}

void cApp::draw(){

    gl::enableAlphaBlending();
    
    mExp.begin( mayaCam.getCamera() ); {
        
        gl::clear( Color(0,0,0) );
        
        for( int w=0; w<mWave.size(); w++ ){
            
            for( int i=0; i<mWave[w].pos.size(); i++ ){
                
                float y = mWave[w].pos[i].y;
                
                float type = randFloat();
                type = abs(type);
                type *= 13.0;
                
                float n = mPln.noise(frame, y, i*0.05);
                n = abs(n)*(800 + randFloat(-1.0f,1.0f)*400) ;
                
                if( i+n > mWave[w].pos.size() ){
                    n = mWave[w].pos.size() - i - 1;
                }
                
                switch ((int)type){
                        
                    case 0:
                        break;
                        
                    case 1:
                        draw_perlin_wave(mWave[w], i, n);
                        break;
                        
                    case 2:
                        draw_line(mWave[w], i, n);
                        break;
                        
                    case 3:
                        draw_line_out(mWave[w], i, n);
                        break;
                        
                    case 4:
                        draw_connect(mWave[w], i, n);
                        break;
                        
                    case 5:
                        draw_rect(mWave[w], i, n);
                        break;
                        
                    case 6:
                        draw_circle(mWave[w], i, n);
                        break;
                        
                    case 7:
                        draw_circle_big(mWave[w], i, n);
                        break;
                        
                    case 8:
                        draw_low_res(mWave[w], i, n);
                        break;
                        
                    case 9:
                    case 10:
                    case 11:
                        draw_dot_wave(mWave[w], i, n);
                        break;
                        
                    case 12:
                        draw_angle(mWave[w], i, n);
                        break;

                    default:
                        draw_line_wave(mWave[w], i, n);
                        break;
                }
                
                i += n;
            }
        }
    } mExp.end();

    gl::clear( Color(0,0,0) );
    glColor3f(1, 1, 1);
    mExp.draw();
}

void cApp::draw_dot_wave( const Wave & wave, int start, int n){
    
    glPointSize(2);
    
    for( int i=0; i<n; i++){
        int id = start + i;
        const Vec3f & pos = wave.pos[id];
        const ColorAf & color = wave.color[id];
        float x = pos.x - mFpb/2;
        float y = pos.y * amp;
        glBegin( GL_POINTS );
        glColor4f( color );
        glVertex2f( x, y );
        glEnd();
    }
}

void cApp::draw_line_wave( const Wave & wave, int start, int n){

    glLineWidth(3);
    glBegin( GL_LINE_STRIP );

    for( int i=0; i<n; i++){
        int id = start + i;
        const Vec3f & pos = wave.pos[id];
        const ColorAf & color = wave.color[id];
        float x = pos.x - mFpb/2;
        float y = pos.y * amp;
        
        glColor4f( color );
        glVertex2f( x, y );
    }
    
    glEnd();
}

void cApp::draw_perlin_wave( const Wave & wave, int start, int n){

    glPointSize(2);
    
    Vec3f pos1 = wave.pos[start];
    Vec3f pos2 = wave.pos[start+n-1];
    const ColorAf & color1 = wave.color[start];
    
    pos1.x -= mFpb/2;
    pos2.x -= mFpb/2;
    pos1.y *= amp;
    pos2.y *= amp;
    
    Vec3f dist = pos2 - pos1;
    
    for( int i=0; i<n*5; i++){
        
        float rx = randFloat() * dist.x;
        float ry = randFloat() * dist.y;
        
        glBegin( GL_POINTS );
        glColor4f( color1 );
        glVertex2f( pos1.x+rx, pos1.y+ry*2 );
        glEnd();
    }
}

void cApp::draw_line( const Wave & wave, int start, int n){
    
    glLineWidth(1);

    for( int i=0; i<n; i++){
        int id = start + i;
        const Vec3f & pos = wave.pos[id];
        const ColorAf & color = wave.color[id];
        float x = pos.x - mFpb/2;
        float y = pos.y;
        
        Vec2f v1(x, y*amp);
        Vec2f v2(x, 0);
        
        glBegin( GL_LINES );
        glColor4f( color );
        glVertex2f( v1 );
        glVertex2f( v2 );
        glEnd();
    }
}

void cApp::draw_line_out( const Wave & wave, int start, int n){
    
    glLineWidth(1);
    
    for( int i=0; i<n; i++){
        int id = start + i;
        const Vec3f & pos = wave.pos[id];
        const ColorAf & color = wave.color[id];
        float x = pos.x - mFpb/2;
        float y = pos.y;
        
        Vec2f v1(x, y*amp);
        Vec2f v2(x, y<0 ?  -amp/2 : amp/2);
        
        glBegin( GL_LINES );
        glColor4f( color );
        glVertex2f( v1 );
        glVertex2f( v2 );
        glEnd();
    }
}

void cApp::draw_angle( const Wave & wave, int start, int n){

    Vec3f piv = wave.pos[start+n/2];
    piv.x -= mFpb/2;
    piv.y *= amp;
    piv.x += randFloat(-1, 1)*10;
    piv.y += randFloat(-1, 1)*22;
    
    glLineWidth(1);
    
    for( int i=0; i<n; i+=3){
        int id = start + i;
        Vec3f pos = wave.pos[id];
        const ColorAf & color = wave.color[id];
        pos.x -= mFpb/2;
        pos.y *= amp;
        
        Vec3f dist = pos - piv;
        
        glBegin( GL_LINES );
        glColor4f( color );
        glVertex3f( piv );
        glVertex3f( piv - dist );
        glEnd();
    }
    
    glBegin( GL_LINES );
    glVertex3f( piv );
    glVertex3f( Vec3f(piv.x, 0, 0) );
    glEnd();

}

void cApp::draw_circle( const Wave & wave, int start, int n){

    Vec3f pos = wave.pos[start+n/2];
    pos.x -= mFpb/2;
    pos.y *= amp;
    Vec2f center(pos.x, pos.y + randFloat(-1,1)*amp/2 );
    
    const ColorAf & color = wave.color[start];
    
    glLineWidth(1);
    glColor4f( color );
    glBegin( GL_LINES );
    glVertex3f( pos );
    glVertex2f( center );
    glEnd();
    
    glPointSize(1);
    gl::drawStrokedCircle( center, 10+randFloat()*20);
}

void cApp::draw_circle_big( const Wave & wave, int start, int n){
    
    Vec3f pos = wave.pos[start+n/2];
    pos.x -= mFpb/2;
    pos.y *= amp;
    Vec2f center(pos.x, pos.y + randFloat(-1,1)*amp/2 );
    
    const ColorAf & color = wave.color[start];

    glLineWidth(1);
    glColor4f( color );
    glBegin( GL_LINES );
    glVertex3f( pos );
    glVertex2f( center );
    glEnd();

    glColor4f( color );
    gl::drawStrokedCircle( center, 50+randFloat()*50);
}

void cApp::draw_rect( const Wave & wave, int start, int n){
    
    const Vec3f & pos1 = wave.pos[start];
    const Vec3f & pos2 = wave.pos[start+n-1];
    const ColorAf & color = wave.color[start];
    
    glColor4f( color );
    gl::drawStrokedRect( Rectf(Vec2f(pos1.x-mFpb/2, pos1.y*amp), Vec2f(pos2.x-mFpb/2, pos2.y*amp)) );
}

void cApp::draw_spline( const Wave & wave, int start, int n){
    
}

void cApp::draw_connect( const Wave & wave, int start, int n){

    const Vec3f & pos1 = wave.pos[start];
    const Vec3f & pos2 = wave.pos[start+n-1];
    const ColorAf & color1 = wave.color[start];
    
    glBegin( GL_LINES );
    glColor4f( color1 );
    glVertex2f( pos1.x-mFpb/2, pos1.y*amp );
    glVertex2f( pos2.x-mFpb/2, pos2.y*amp);
    glEnd();
}

void cApp::draw_low_res( const Wave & wave, int start, int n){

    glLineWidth(1);
    glBegin( GL_LINE_STRIP );
    
    for( int i=0; i<n; i+=30){
        int id = start + i;
        const Vec3f & pos = wave.pos[id];
        const ColorAf & color = wave.color[id];
        float x = pos.x - mFpb/2;
        float y = pos.y * amp * i==0?1:2;
        
        glColor4f( color );
        glVertex2f( x, y );
    }

    int id = start + n-1;
    const Vec3f & pos = wave.pos[id];
    const ColorAf & color = wave.color[id];
    float x = pos.x - mFpb/2;
    float y = pos.y * amp;
    glColor4f( color );

    glVertex2f( x, y );
    glEnd();
    
    
}

void cApp::draw_fan( const Wave & wave, int start, int n){
    
}

void cApp::draw_log( const Wave & wave, int start, int n){
    
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
