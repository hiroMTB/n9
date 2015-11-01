//#define RENDER
#define HARF

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
#include "mtUtil.h"
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
    
    void draw_grid();
    void draw_gridExp();
    void draw_gridPln();
    void draw_graph();

    void draw_waveExp();
    void draw_wavePln();
    
    void draw_angle();
    void loadColorSample( string fileName, vector<vector<Colorf>>& col );
    
    int win_w = 4320;
    int win_h = 1920;

    Perlin mPln;
    vector<Wave> mWaves;
    vector<vector<Colorf>> mColorSample1;

    Exporter mExp;
  
    int frame = -1;
    float fftWaveSamplingRate = 6000;
    int mFpb = 2048*2*2;
    
    MayaCamUI mayaCam;
    
    int audioPos = 0;//192000 * 90;
    float yoffset = 0;
    int jump = 0;

};

void cApp::setup(){
    
#ifdef HARF
    win_h *= 0.5;
    win_w *= 0.5;
    setWindowSize( win_w, win_h);
#elif
    setWindowSize( win_w/2, win_h/2);
#endif
    
    setFrameRate(25);
    setWindowPos( 0, 0 );
    gl::enableVerticalSync();
    
    mPln.setOctaves(4);
    mPln.setSeed(444);
    
    mExp.setup( win_w, win_h, 1000, GL_RGB, mt::getRenderPath(), 0 );
    mt::loadColorSample("img/geo/Mx80_2_org.jpg", mColorSample1 );
    
    mWaves.assign( 8, Wave() );
    
    for( int i=0; i<mWaves.size(); i++ ){
        mWaves[i].create( "snd/samples/192k/3s1e_192k_" + toString(i+1) + ".wav");
    }

    CameraPersp persp;
    persp.setAspectRatio( win_w/win_h);
    persp.setNearClip(1);
    persp.setFarClip(100000);
    persp.lookAt( Vec3f(0,0,-18000), Vec3f(0,0,0), Vec3f(0,1,0) );
    mayaCam.setCurrentCam( persp );
    
#ifdef RENDER
    mExp.startRender();
#endif

}

void cApp::update(){

    frame++;
    int nFrame = mWaves[0].buf->getNumFrames();
    audioPos +=mFpb;
    
    if( audioPos >= nFrame ){
        cout << "Quit App, frame: " << frame << "\n"
        << "audio pos: " << audioPos << ", audio total frame : " << nFrame << endl;
        quit();
    }
    
    for( int w=0; w<mWaves.size(); w++ ){
        
        const float * ch0 = mWaves[w].buf->getChannel( 0 );

        mWaves[w].update(audioPos, mFpb);
        
        // color
        for( int i=0; i<mWaves[w].color.size(); i++ ){
            ColorAf & oldColor = mWaves[w].color[i];
            Vec3f noise = mPln.dfBm( getElapsedFrames()*0.2, i*0.2, mWaves[w].chL[i]*0.2 );
            noise *= 0.1;
            oldColor.r += noise.x;
            oldColor.g += noise.z;
            oldColor.b += noise.y;
            oldColor.a = 0.8;
        }
    }
}

void cApp::draw(){

    gl::enableAlphaBlending();

    mExp.begin( mayaCam.getCamera() ); {
        gl::clear( Color(0,0,0) );
        //draw_grid();
        //draw_gridExp();
        //draw_gridPln();
        draw_wavePln();
        
        //draw_waveExp();
        //draw_graph();
        
    } mExp.end();
    
    gl::clear( Color(0,0,0) );
    glColor3f(1, 1, 1);
    mExp.draw();
}

void cApp::draw_waveExp(){
    glColor3f(0, 1, 0);
    
    double ang = toRadians( (double)(getElapsedFrames()%(360*4)) );
    float exponential = 1.001 + abs(sin( ang )) * 6.0;
    
    for( int w=0; w<mWaves.size(); w++ ){
  
        Wave & wave = mWaves[w];
        

        vector<Vec2d> point;
        for(int i=0; i<wave.chL.size(); i++){

            double x = pow(exponential, abs(i-mFpb/2)) - 1.0f;

            if( i<mFpb/2 ){
                x = -x;
            }
            
            x *=0.1f;
            
            double y = wave.chL[i];
            y *= 2000;
            point.push_back(Vec2d(-x, y));
            
            // guid
            if(1){
                glLineWidth(1);
                glColor4f(0,0,1, 1);
                glBegin(GL_LINES);
                glVertex2d( -x, -1100 );
                glVertex2d( -x, -1000 );
                glEnd();
            }
        }

        glLineWidth(1);
        glColor3f(0,1,0);
        glBegin(GL_LINE_STRIP);
        for( auto & p : point ){
            glVertex2f( p );
        }
        glEnd();
        
        
        glPointSize(3);
        glColor3f(1,0,0);
        glBegin(GL_POINTS);
        for( auto & p : point ){
            glVertex2f( p );
        }
        glEnd();
    }
}

void cApp::draw_wavePln(){
    
    for( int w=0; w<mWaves.size(); w++ ){
        
        Wave & wave = mWaves[w];
        
        double x = -12000;
        vector<Vec2d> point;
        for(int i=0; i<wave.chL.size(); i++){
            
            Vec3f noise = mPln.dfBm(getElapsedFrames()*0.0001, w+0.1, i*0.1);
            noise.x += 1.0;
            noise.y += 1.0;
            noise.z += 1.0;
            float sum = noise.x + noise.y + noise.z;
            sum /= 3.0;
            float px = pow(sum,25 );
            x += px*0.006;
            double y = wave.chL[i];
            
            if( px >1.9 ){
                y = (y>0?1:-1) * -2200;
                
                if( jump==0 ){
                    y = (y>0?1:-1) * -2200;
                    yoffset = (y>0?1:-1) * -600;
                    jump = 300;
                }else{
                    jump--;
                }
            }else{
                y *= 2000;
            }
            point.push_back(Vec2d(-x, y+yoffset));
            
            // guid
            if(0){
                glLineWidth(1);
                glColor4f(0,0,1, 1);
                glBegin(GL_LINES);
                glVertex2d( -x, -1100 );
                glVertex2d( -x, -1000 );
                glEnd();
            }
        }
        
        glLineWidth(1);
        glColor4f(0,0,1, 0.5);
        glBegin(GL_LINE_STRIP);
        for( auto & p : point ){
            glVertex2f( p );
        }
        glEnd();
        
        
        glPointSize(1);
        glColor3f(1,0,0);
        glBegin(GL_POINTS);
        for( auto & p : point ){
            glVertex2f( p );
        }
        glEnd();
    }

}

void cApp::draw_graph(){

    // vertical line
    int nStep = 20;
    double x = 0;
    double adder = 0.02;
    double adder2 = 1.2;
    
    glPointSize(4);
    glColor4f(1, 0, 0, 0.8);
    glBegin( GL_POINTS );
    for( int i=0; i<nStep; i++ ){
        
        double xS =  x * 0.1;
        glVertex2d(  -i*20, xS);
        x += adder;
        adder2 += 0.1;
        adder *= adder2;
    }
    glEnd();
    
    
    
    // EXP
    glPointSize(4);
    glColor4f(0, 1, 0, 0.8);
    glBegin( GL_POINTS );
    for( int i=0; i<nStep; i++ ){
        
        double xS =  pow(1.5, i) * 10;
        glVertex2d(  -i*20, xS);
        x += adder;
        adder2 += 0.1;
        adder *= adder2;
    }
    glEnd();
    
}

void cApp::draw_grid(){

    glColor3f(1, 0, 0);
    
    glPushMatrix(); {
        double scale_x = win_w/2;
        double scale_y = win_h/2;
        
        glColor4f(1, 0, 0, 0.8);
        
        double offset_x = 0;
        
        if(1){
            // vertical line
            int nStep = 20;
            double x = 0;
            double adder = 0.01;
            double adder2 = 1.1;

            glBegin( GL_LINES );
            for( int i=0; i<nStep; i++ ){
                
                double xS = scale_x*x;
                glVertex2d(  -xS + offset_x,  scale_y);
                glVertex2d(  -xS + offset_x, -scale_y);
                glVertex2d(  xS  + offset_x,  scale_y);
                glVertex2d(  xS  + offset_x, -scale_y);
                
                x += adder;
                adder2 += 0.1;
                adder *= adder2;
            }
            glEnd();

        }
        
        float offset_y = 0;

        if(1){
            // horizontal line
            int nStep = 20;
            float y = 0;
            float adder = 0.01f;
            float adder2 = 1.1f;

            glBegin( GL_LINES );
            for( int i=0; i<nStep; i++ ){
                
                float yS = scale_y*y;
                glVertex3f(  scale_x,  yS + offset_y, 0);
                glVertex3f( -scale_x,  yS + offset_y, 0);
                glVertex3f(  scale_x, -yS + offset_y, 0);
                glVertex3f( -scale_x, -yS + offset_y, 0);
                y += adder;
                adder2 += 0.1;
                adder *= adder2;
            }
            glEnd();
        }
    } glPopMatrix();
}

void cApp::draw_gridExp(){
    
    glColor3f(0, 1, 0);
    
    glPushMatrix(); {
        double scale_x = win_w/2;
        double scale_y = win_h/2;
        
        glColor4f(0, 0, 1, 0.2);
        
        double offset_x = 0;
        
        if(1){
            // vertical line
            int nStep = 20;
            
            glBegin( GL_LINES );
            
            // ZERO
            glVertex2d(  0 + offset_x,  scale_y);
            glVertex2d(  0 + offset_x,  -scale_y);
            
            for( int i=0; i<nStep; i++ ){
                
                double xS = pow(1.5, i)*10.0f - 10.0f;
                glVertex2d(  -xS + offset_x,  scale_y);
                glVertex2d(  -xS + offset_x, -scale_y);
                glVertex2d(  xS  + offset_x,  scale_y);
                glVertex2d(  xS  + offset_x, -scale_y);
            }
            glEnd();
        }
        
        float offset_y = 0;
        
        if(0){
            // horizontal line
            int nStep = 20;
            
            glBegin( GL_LINES );
            for( int i=0; i<nStep; i++ ){
                
                float yS = pow(1.5, i)*10.0f - 10.0f;
                glVertex3f(  scale_x,  yS + offset_y, 0);
                glVertex3f( -scale_x,  yS + offset_y, 0);
                glVertex3f(  scale_x, -yS + offset_y, 0);
                glVertex3f( -scale_x, -yS + offset_y, 0);
            }
            glEnd();
        }

    } glPopMatrix();
}

void cApp::draw_gridPln(){
    
    glColor4f(0, 0, 1, 1);
    glLineWidth(1);
    
    glPushMatrix(); {
        double scale_x = win_w/2;
        double scale_y = win_h/2;
        
        
        double offset_x = scale_x*12;
        double x = 0;
        
        if(1){
            // vertical line
            int nStep = 4000;
            
            glBegin( GL_LINES );
            
            for( int i=0; i<nStep; i++ ){
                
                Vec3f noise = mPln.dfBm(0.1, 0.1, i*0.1);
                noise.x += 1.0;
                noise.y += 1.0;
                noise.z += 1.0;
                x += pow((noise.x + noise.y + noise.z),10 ) * 0.0001f;
                
                glVertex2d(  -x + offset_x,  scale_y);
                glVertex2d(  -x + offset_x, -scale_y);
            }
            glEnd();
        }
        
        float offset_y = 0;
        
        if(0){
            // horizontal line
            int nStep = 20;
            
            glBegin( GL_LINES );
            for( int i=0; i<nStep; i++ ){
                
                float yS = pow(1.5, i)*10.0f - 10.0f;
                glVertex3f(  scale_x,  yS + offset_y, 0);
                glVertex3f( -scale_x,  yS + offset_y, 0);
                glVertex3f(  scale_x, -yS + offset_y, 0);
                glVertex3f( -scale_x, -yS + offset_y, 0);
            }
            glEnd();
        }
        
    } glPopMatrix();

    
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
