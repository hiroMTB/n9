#define RENDER

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"

#include "cinder/audio/Context.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"
#include "cinder/ImageIo.h"
#include "cinder/Perlin.h"
#include "cinder/rand.h"
#include "cinder/BSpline.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"

#include "mtUtil.h"
#include "mtSvg.h"
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
    void draw_wave();
    
    const int win_w = 4320;
    const int win_h = 1920;

    //vector<Perlin> mPlns;
    //vector<Wave> mWave;
    Exporter mExp;
    fs::path assetDir;
  
    Wave m2mix;
    Perlin mPln[2];
    
    int frame       = 225-1;
    int mFpb        = 1920*2*2;   //48kHz/25fps = 1920
    float camDist   = -2800;
    float amp       = 250;
    int resolution  = 1;
    int totalMovFrame;
    int xoffset = 120;
    
    mtSvg guide;
    bool bInit = false;
};

void cApp::setup(){
    
    setFrameRate(60);
    setWindowSize( win_w*0.5, win_h*0.5 );
    setWindowPos( 0, 0 );
    gl::enableVerticalSync();
    assetDir = mt::getAssetPath();
    guide.load( assetDir/"svg"/"n5_guide.svg" );

    m2mix.create( (assetDir/"snd"/"timeline"/"n5_2mix_192k.aif"), mFpb );
    mPln[0].setOctaves(4);
    mPln[0].setSeed(123.4);
    mPln[1].setOctaves(6);
    mPln[1].setSeed(339.1);

    /*
    for( int w=0; w<1; w++ ){
        mWave.push_back( Wave() );
        //string filename = "n5_2mix_sepf_0" + to_string(w+1)+ ".aif";
        //mWave[w].create( (assetDir/"snd"/"timeline"/"sepf"/"48kHz"/filename) );
        
        mPlns.push_back(Perlin());
        mPlns[mPlns.size()-1].setOctaves(1+w%4);
        mPlns[mPlns.size()-1].setSeed(123*w*w);
    }
     */

    int nFrame = m2mix.buf->getNumFrames();
    totalMovFrame = nFrame / mFpb;
    
    mExp.setup( win_w, win_h, 0, totalMovFrame, GL_RGB, mt::getRenderPath(), 0 );
    
#ifdef RENDER
    mExp.startRender();
#endif

}

void cApp::update(){

    frame++;
    
    int nFrame = m2mix.buf->getNumFrames();
    long audioPos = frame * mFpb;
    
    if( audioPos >= nFrame ){
        cout << "Quit App, frame: " << frame << "\n"
        << "audio pos: " << audioPos << ", audio total frame : " << nFrame << endl;
        quit();
    }
    
    m2mix.updatePos( audioPos );
    
}

void cApp::draw(){

    gl::enableAlphaBlending();
    
    mExp.beginPersp(); {
        gl::clear( Color(0,0,0) );
        glPushMatrix();
        gl::translate(mExp.mFbo.getWidth()/2, mExp.mFbo.getHeight()/2);
        draw_wave();
        glPopMatrix();
        
        if( bInit )
        {
            guide.draw();
            bInit = true;
        }
        
    } mExp.end();

    gl::clear( Color(0,0,0) );
    glColor3f(1, 1, 1);
    mExp.draw();
    
}

void cApp::draw_wave(){
  
    // draw Point
    {
        float canvasWidth = mExp.mFbo.getWidth()-xoffset*2;
        int sampleNum = mFpb/resolution;
        float sampleDistance = (float)canvasWidth/sampleNum;
        
        for( int c=0; c<m2mix.numCh; c++ ){

            Vec2f pen(0,0);
            Vec2f prepSmp(0,0);
            int penChangeFrame = 0;
            
            for ( int i=0; i<sampleNum; i++) {
                
                Vec2f & smp = m2mix.pos[c][i];
                ColorAf & col = m2mix.col[c][i];
                
                smp.x *= sampleDistance;
                smp.x -= mExp.mFbo.getWidth()/2;
                smp.x += xoffset;
                
                smp.y *= amp;
                smp.y += 0;
                if(c==0) smp.y = -smp.y;
                
                // color segment
                Vec3f noise = mPln[c].dfBm(c+3, c+frame*0.0006/2, i*0.00061/2 );
                
                float seg = noise.x+noise.y+noise.z;
                seg = seg*seg;
                
                if( seg < 0.002){
                    // RED NORMAL WAVE
                    col = ColorAf(1,0,0,1);
                    glColor4f(col);
                    glPointSize(1);
                    glBegin(GL_POINTS);
                    float x = smp.x + pen.x;
                    float y = -smp.y*2 + pen.y;
                    glVertex2f( x, y );
                    glEnd();
                }
                else if( seg < 0.07 ){
                    // WHITE LINE WAVE
                    col = ColorAf(1,1,1,0.7);
                    glColor4f(col);
                    glLineWidth(1);
                    glBegin(GL_LINES);
                    glVertex2f( smp +pen );
                    glVertex2f( Vec2f(smp.x+pen.x,0)  );
                    glEnd();
                }
                else if( seg < 0.3 ){
                    // Linear Jump
                    Vec2f prepPen = pen;
                    if( i-penChangeFrame > mFpb*0.2 ){
                        //float newPenx = (noise.z*noise.y) * -1000;
                        float newPeny = (noise.x*noise.y) * -170;
                        //pen.x = prepPen.x*0.01 + newPenx*0.99;
                        pen.y = prepPen.y*0.01 + newPeny*0.99;

                        penChangeFrame = i;
                        col = ColorAf(1,0,1,1);
                        glColor4f(col);
                        glBegin(GL_LINES);
                        glVertex2f( prepSmp + prepPen);
                        glVertex2f( smp + pen);
                        glEnd();
                        prepPen = pen;
                    }
                    
                    col = ColorAf(1,0,1,1);
                    glColor4f(col);
                    glBegin(GL_POINTS);
                    glVertex2f( smp + pen);
                    glEnd();
                }
                else if( seg < 0.7 ){
                    // LOW PASS, WATERBLUE
                    col = ColorAf(0,0.5,0.5,1);
                    glPointSize(1);
                    glColor4f(col);
                    glBegin(GL_POINTS);
                    smp.y = smp.y*0.02 + prepSmp.y*0.98;
                    glVertex2f( smp.x+pen.x, smp.y*(noise.y) +pen.y );
                    glEnd();
                }
                else if( seg < 0.8 ){
                    // WHITE LINE WAVE
                    if(i%2==0){
                        col = ColorAf(1,1,1,0.5);
                        glColor4f(col);
                        glLineWidth(1);
                        glBegin(GL_LINES);
                        float x = smp.x + pen.x;
                        float y = smp.y + pen.y;
                        glVertex2f( x, y );
                        glVertex2f( x, y*1.4 );
                        glEnd();
                    }
                }
                else if( seg < 1.0 ){
                    // Sprine Jump, BLUE
                    Vec2f prepPen = pen;
                    if( i-penChangeFrame > mFpb*0.2 ){
                        float newPenx = (noise.z*noise.y) * -1400;
                        float newPeny = (noise.x*noise.y) * -1400;
                        pen.x = prepPen.x*0.01 + newPenx*0.99;
                        pen.y = prepPen.y*0.01 + newPeny*0.99;
                        
                        penChangeFrame = i;
                        col = ColorAf(0,0,1,1);
                        
                        vector<Vec2f> ctrls;
                        Vec2f st = prepSmp + prepPen;
                        Vec2f end = smp + pen;
                        Vec2f dir = end - st;
                        Vec2f mid1 = st + dir*0.33;
                        Vec2f mid2 = st + dir*0.66;
                        mid1.y += dir.y*abs(noise.x)*0.5;
                        mid2.y += dir.y*abs(noise.y);

                        ctrls.push_back( st );
                        ctrls.push_back( mid1 );
                        ctrls.push_back( mid2 );
                        ctrls.push_back( end );
                        BSpline2f sp(ctrls, 3, false, true);
                        
                        glColor4f(col);
                        glBegin(GL_LINES);
                        int resolution = 30;
                        for( int j=0; j<resolution-1; j++ ){
                            float rate1 = (float)j/resolution;
                            float rate2 = (float)(j+1)/resolution;
                            
                            Vec2f v1 = sp.getPosition( rate1 );
                            Vec2f v2 = sp.getPosition( rate2 );
                            
                            glVertex2f( v1 );
                            glVertex2f( v2 );
                        }
                        glEnd();

                        prepPen = pen;
                    }
                    
                    col = ColorAf(1,0,1,1);
                    glColor4f(col);
                    glBegin(GL_POINTS);
                    glVertex2f( smp + pen);
                    glEnd();
                }
                
                else{
                    col = ColorAf(1,1,1,0.5);
                    glPointSize(1);
                    glColor4f(col);
                    glBegin(GL_POINTS);
                    glVertex2f( smp + pen);
                    glEnd();
                }
                
                prepSmp = smp;
            }
        }
    }
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )







