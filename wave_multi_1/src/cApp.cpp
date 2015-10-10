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

#include "ufUtil.h"
#include "Wave.h"
#include "Resources.h"
#include "Exporter.h"

//#define RENDER

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
    void loadColorSample( string fileName, vector<vector<Colorf>>& col );
    

    const int win_w = 4320;
    const int win_h = 1920;

    vector<Wave> mWaves;
    vector<vector<Colorf>> mColorSample1;
    vector<vector<Colorf>> mColorSample2;

    Exporter mExp;
    Exporter mExp2;

    Exporter mBlurCanvas;
    gl::GlslProgRef	mShader;
    float mAngle;
    bool mShaderOn = false;
//    gl::TextureRef mTexture;
    
};

void cApp::setup(){
    
    setFrameRate(25);
    setWindowSize( win_w*0.5, win_h*0.5 );
    setWindowPos( 0, 0 );
    gl::enableVerticalSync();
    
    {
        // Visual Setup
        mShader = gl::GlslProg::create( loadResource( RES_PASSTHRU_VERT ), loadResource( RES_BLUR_FRAG ) );
        mBlurCanvas.setup( getWindowWidth(), getWindowHeight(), 1, GL_RGBA8, "/fake", 0);
        mExp.setup( getWindowWidth(), getWindowHeight(), 3000, GL_RGB, uf::getRenderPath( "line" ), 0);
        mExp2.setup( getWindowWidth(), getWindowHeight(), 3000, GL_RGB, uf::getRenderPath( "point" ), 0);
        
        uf::loadColorSample("img/Mx80_2_org_B.jpg", mColorSample1 );
        uf::loadColorSample("img/Mx80_2_org_B.jpg", mColorSample2 );
    }
    
    {
        // Audio Setup
        auto ctx = audio::Context::master();
        audio::DeviceRef device = audio::Device::getDefaultOutput();
        audio::Device::Format format;
        format.sampleRate(192000);
        format.framesPerBlock(1024*4);
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
    
   	mAngle = 0.0f;
    
#ifdef RENDER

    //mExp.startRender();
    //mExp2.startRender();
#endif

}

void cApp::update(){
    for( int i=0; i<mWaves.size(); i++ ){
        mWaves[i].update( Vec2f(0.5f, 80*(i+1)) );
        
        int blockw = 64;
        
        int fx = randInt(0, mColorSample1.size() - blockw);
        int fy = randInt(0, mColorSample1[0].size() -blockw);
        
        for( int j=0; j<blockw; j++ ){
            for( int k=0; k<blockw; k++ ){
                mWaves[i].colorL[j*blockw+k] =  mColorSample1[fx+j][fy+k];
                mWaves[i].colorR[j*blockw+k] =  mColorSample2[fx+j][fy+k];
            }
        }
    }
    
  	mAngle += 0.05f;
}

void cApp::draw(){

    gl::clear( Color(0,0,0) );

    mBlurCanvas.begin();
    {
        glPushMatrix();
        gl::clear( Color(0,0,0) );
        
        glPointSize(1);
        glLineWidth(1);

        glTranslatef( getWindowWidth()/2-4096/2/2, getWindowHeight()/2, 0 );
        
        for ( int n=0; n<200; n++) {
            for ( int i=0; i<mWaves.size(); i++) {
                for ( int j=i; j<mWaves.size(); j++) {
                    
                    int nVerts = mWaves[i].vboL.getNumVertices();

                    int range = randInt(3, 40);
                    int id1 = randInt(0, nVerts);
                    int id2 = randInt( MAX(0,id1-range), MIN(nVerts, id1+range) );
                    
                    Vec3f p1 = mWaves[i].posL[id1];
                    Vec3f p2 = mWaves[j].posL[id2];
                    
                    ColorAf c = mWaves[i].colorL[id1];
                    
                    glColor4f( c.b, c.g, c.r, c.a*0.06 );
                    glBegin( GL_LINES );
                    glVertex3f( p1.x, p1.y, p1.z );
                    glVertex3f( p2.x, p2.y, p2.z );
                    glEnd();
                }
            }
        }
        
        glPopMatrix();
    }
    mBlurCanvas.end();

    if( 0 ){
        gl::clear();
        gl::TextureRef mTexture = make_shared<gl::Texture>(mBlurCanvas.mFbo.getTexture());
        mTexture->enableAndBind();
        mShader->bind();
        mShader->uniform( "tex0", 0 );
        mShader->uniform( "sampleOffset", Vec2f( cos( mAngle ), sin( mAngle ) ) * ( 3.0f / mBlurCanvas.mFbo.getWidth() ) );
        gl::drawSolidRect( mBlurCanvas.mFbo.getBounds() );
        mTexture->unbind();
        mShader->unbind();
    }
    
//    mExp.begin();
//    glColor3f( 1,1,1 );
//    mBlurCanvas.draw();
//    mExp.end();
//
//    glColor3f( 1,1,1 );
//    mExp.draw();
    

    mExp2.begin();
    gl::clear( Color(0,0,0) );
    glPushMatrix();
    {
        glTranslatef( getWindowWidth()/2-4096/2/2, getWindowHeight()/2, 0 );
        for( int i=0; i<mWaves.size(); i++ ){
            mWaves[i].drawL();
            mWaves[i].drawR();
        }
    }
    glPopMatrix();
    mExp2.end();
         
}

void cApp::keyDown( KeyEvent event ){
    mShaderOn = !mShaderOn;
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
