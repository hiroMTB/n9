#define RENDER

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/rand.h"
#include "cinder/Perlin.h"

#include "Exporter.h"
#include "ufUtil.h"
#include "Wave.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();

    void keyDown( KeyEvent event );
    void mouseMove( MouseEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void resize();
    
    Exporter mExp;
    
    const int mW        = 4320;     // 1080*4
    const int mH        = 1920;
    const float mScale  = 0.5;
    const int mFpb       = 1024*4;   // frames per block(audio buffer)
    
    vector<vector<Colorf>> mColorSample1;
    vector<vector<Colorf>> mColorSample2;

    vector<Wave> mWaves;
    Perlin mPln;
    
    Vec2f pivL, pivR;
    int inpoint;
    int outpoint;
    
    float amp;
};

void cApp::setup(){
    setFrameRate( 25 );
    setWindowPos( 0, 0 );
    setWindowSize( mW*mScale, mH*mScale );
    mExp.setup( mW*mScale, mH*mScale, 2999, GL_RGB, uf::getRenderPath(), 0, true);
    
    mPln.setOctaves( 3 );
    mPln.setSeed( 551 );
    
    uf::loadColorSample("img/Mx80_2_org_B.jpg", mColorSample1 );
    uf::loadColorSample("img/Mx80_2_org_B.jpg", mColorSample2 );

    
    {
        // Audio Setup
        auto ctx = audio::Context::master();
        audio::DeviceRef device = audio::Device::getDefaultOutput();
        audio::Device::Format format;
        format.sampleRate( 192000 );
        format.framesPerBlock( mFpb );
        device->updateFormat( format );
        
        cout << "--- Audio Setting --- " << endl;
        cout << "device name      : " << device->getName() << endl;
        cout << "Sample Rate      : " << ctx->getSampleRate() << endl;
        cout << "frames per Block : " << ctx->getFramesPerBlock() << endl;

        mWaves.assign( 7, Wave() );
        
        for( int i=0; i<mWaves.size(); i++ ){
            mWaves[i].create( "snd/test/3s1e_192k_" + toString(i+1)+ ".wav" );
        }

        ctx->enable();
    }
    
    
    pivL.set( 200, getWindowHeight()/2 );
    pivR.set( getWindowWidth()-200, getWindowHeight()/2 );
    
    amp = 0.1f;
    
#ifdef RENDER
    mExp.startRender();
#endif

}

void cApp::update(){

    const int frame = getElapsedFrames()-1;
    const int audioPos = frame * 192000.0f/25.0f;
    
    int readPos = mWaves[0].player->getReadPosition();
    
    //cout << "readPos  : " << readPos << endl;
    //cout << "audioPos : " << audioPos << endl;
    //cout << "diff     : " << readPos - audioPos << endl << endl;
    
    for( int w=0; w<mWaves.size(); w++ ){
        
        //mWaves[w].player->seek(audioPos);
        
        // Update Wave pos
        Vec2f scale(0.25f, 200);

        const float * ch0 = mWaves[w].buf->getChannel( 0 );
        const float * ch1 = mWaves[w].buf->getChannel( 1 );
        
        for ( int i=0; i<mFpb; i++) {
            float l = ch0[audioPos + i];
            float r = ch1[audioPos + i];

            Vec3f newL = Vec3f(i*scale.x, l*scale.y * amp, 0);
            Vec3f newR = Vec3f(i*scale.x, r*scale.y * amp, 0);

            if(i==0){
                mWaves[w].posL[i] = newL;
                mWaves[w].posR[i] = newR;
            }else{
                mWaves[w].posL[i] = mWaves[w].posL[i-1]*0.99f + newL*0.01f;
                mWaves[w].posR[i] = mWaves[w].posR[i-1]*0.99f + newR*0.01f;
            }
        }
        
        // Update Wave Color
        int sk = 8;
        int blockw = 64;
        int fx = randInt(0, mColorSample1.size() - blockw*sk);
        int fy = randInt(0, mColorSample1[0].size() -blockw*sk);
        
        for( int j=0; j<blockw; j++ ){
            for( int k=0; k<blockw; k++ ){
                mWaves[w].colorL[j*blockw+k] =  mColorSample1[fx+j*sk][fy+k*sk];
                mWaves[w].colorR[j*blockw+k] =  mColorSample2[fx+j*sk][fy+k*sk];
                mWaves[w].colorL[j*blockw+k].a = 0.8;
                mWaves[w].colorR[j*blockw+k].a = 0.8;
            }
        }
    }
    
    
    inpoint = mFpb/2 - frame*4.0f + mPln.noise( randInt(), getElapsedFrames()*0.01 ) * 200 * amp;
    outpoint = mFpb/2 + frame*4.0f + mPln.noise( randInt(), getElapsedFrames()*0.01 ) * 200 * amp;
    amp += 0.003;
    
    
    amp = MIN( amp, 1.3f );
    inpoint = MAX( inpoint, 0);
    outpoint = MIN( outpoint, mFpb-1 );
    
}

void cApp::draw(){

    float transx = getWindowWidth()/2 - mFpb/2/2/2;
    float transy = getWindowHeight()/2;
    
    mExp.begin();
    {
        gl::clear( Colorf(0,0,0) );
        
        glPushMatrix();
        {
            glTranslatef( transx, transy, 0 );
            
            for( int w=0; w<mWaves.size(); w++ ){

                for( int i=0; i<mWaves[w].posL.size(); i++ ){
                    
                    bool range = inpoint<=i && i<=outpoint;
                    if( !range ) continue;
                    
                    float noise = mPln.noise( mWaves[w].posR[i].y*mWaves[w].posL[i].y*0.5, i*0.5 );
                    if( noise<0.67 ){
                        glPointSize( 1 );
                    }else if( noise<0.72 ){
                        glPointSize( 2 );
                    }else if( noise<0.8 ){
                        glPointSize( 3 );
                    }else{
                        glPointSize( 1 );
                        gl::drawStrokedCircle( Vec2f(mWaves[w].posL[i].x, mWaves[w].posL[i].y), MAX(1, (noise-0.8f)*200.0f) );
                    }
                    
                    if(randFloat()<0.97f) glColor3f( mWaves[w].colorL[ i ].r, mWaves[w].colorL[ i ].g, mWaves[w].colorL[ i ].b );
                    else glColor3f(1, 0, 0);

                    glBegin( GL_POINTS );
                    glVertex2f( mWaves[w].posL[i].x,  mWaves[w].posL[i].y );
                    glEnd();
                    
                    if(randFloat()<0.97f) glColor3f( mWaves[w].colorR[i].r, mWaves[w].colorR[i].g, mWaves[w].colorR[i].b );
                    else glColor3f(1, 0, 0);

                    glBegin( GL_POINTS );
                    glVertex2f( mWaves[w].posR[i].x, mWaves[w].posR[i].y );
                    glEnd();
                }
            }
        }
        glPopMatrix();
        
        glPushMatrix();
        {
            for( int w=0; w<mWaves.size(); w++ ){

                const Wave & mWave = mWaves[w];
                
                // pivot
                glPointSize( 3 );
                glColor3f(1, 0, 0);
                glBegin( GL_POINTS );
                glVertex2f( pivL.x, pivL.y );
                glVertex2f( pivR.x, pivR.y );
                glVertex2f( mWave.posL[inpoint].x+transx, mWave.posL[inpoint].y+transy );
                glVertex2f( mWave.posL[outpoint].x+transx,  mWave.posL[outpoint].y+transy );
                
                glEnd();
                
                // handle
                glLineWidth( 1 );
                glColor3f(1, 0, 0);
                glBegin( GL_LINES );
                
                glVertex2f( pivL.x, pivL.y );
                glVertex2f( mWave.posL[inpoint].x+transx, mWave.posL[inpoint].y+transy );
                
                glVertex2f( pivL.x, pivL.y );
                glVertex2f( mWave.posR[inpoint].x+transx, mWave.posR[inpoint].y+transy );
                
                
                glVertex2f( pivR.x, pivR.y );
                glVertex2f( mWave.posR[outpoint].x+transx,  mWave.posR[outpoint].y+transy );

                glVertex2f( pivR.x, pivR.y );
                glVertex2f( mWave.posL[outpoint].x+transx,  mWave.posL[outpoint].y+transy );

                
                glEnd();
            }
            
            
        }
        glPopMatrix();
        
    }
    mExp.end();
    

    gl::clear( Colorf(1,1,1) );
    gl::color( Colorf(1,1,1) );
    mExp.draw();
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


