//#define RENDER
//#define PROXY
#define CALC_3D

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

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace tbb;

class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    void task(float i );
    
    void keyDown( KeyEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    
    Exporter mExp;
    int mW;
    int mH;
    bool bOrtho = false;
    bool bShowNoise = false;
    
    VboSet vbo;
    vector<Vec3f> spd;
    vector<Vec3f> dest;
    vector<Perlin> mPln;
    vector<Vec2i> absPos = { Vec2i(1640,192), Vec2i(2680,192), Vec2i(1640,1728), Vec2i(2680,1728) };

    float dispScale = 0.5;
    double frame    = 0;
    Vec2f noiseCenter;
    
    fs::path assetDir = mt::getAssetPath();
};

void cApp::setup(){

#ifdef PROXY
    mW = 1080*4/2;
    mH = 1920/2;
    setWindowSize( mW, mH );
#else
    mW = 1080*4;
    mH = 1920;
    setWindowSize( mW*dispScale, mH*dispScale );
#endif
    setWindowPos( 0, 0 );
    mExp.setup( mW, mH, 0, 1000-1, GL_RGB, mt::getRenderPath(), 0);
    
    mPln.push_back( Perlin(4, 132051) );

    noiseCenter.x = mW/2;
    noiseCenter.y = mH/2;
    noiseCenter.x = 1802 / dispScale;
    noiseCenter.y =  347 / dispScale;
    
    int cnt = 0;
    
    Surface img = Surface( loadImage( assetDir/"img"/"redCloud"/"redCloud01.png" ));
    int iW = img.getWidth();
    int iH = img.getHeight();
    
    for (int y=0; y<mH; y+=4) {
        for (int x=0; x<mW; x+=4) {

            Vec3f p(x, y, 0 );
            p.x += noiseCenter.x-mW/2;
            p.y += noiseCenter.y-mH/2;
            
            vbo.addPos( p );

            Vec3f vel = mPln[0].dfBm(x*0.005, y*0.005, 0 );
            spd.push_back( vel );

            cnt++;
            
            float n = mPln[0].noise( x/dispScale, y/dispScale, cnt*0.1);
            float a = n < 0.5 ? 1 : 0;
            vbo.addCol( ColorAf(1,0,0,a) );

        }
    }
    vbo.init( GL_POINTS, true, true );
    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){

    int loadNum = vbo.getPos().size() * MIN(frame/100.0, 1.0);

    if( 1 ){
        parallel_for( blocked_range<size_t>(0, loadNum), [=](const blocked_range<size_t>& r) {
            for( size_t i=r.begin(); i!=r.end(); ++i ){
                task( i );
            }
        });
    }else{
        for( int s=0; s<loadNum; s++){
            task( s );
        }
    }
    
    vbo.updateVboPos();
    vbo.updateVboCol();
}


void cApp::task( float i  ){
    
    double ox = 6;
    double oy = 4.2;
    
    double resRate = 0.2;
    
    Vec3f p = vbo.getPos()[i];
    Vec3f n = mPln[0].dfBm( ox+p.x*0.001*resRate, oy+p.y*0.001*resRate, 2.1+frame*0.00001 );
    
    double rate = (frame+1)/300.0 + 0.2;
    if(rate>1){
        rate = pow( rate, 3);
        rate = MIN( rate, 1000);
    }
    
    float oRate = MIN(rate, 1.0);

    spd[i].x = spd[i].x * (1.0-oRate) + n.x*rate;
    spd[i].y = spd[i].y * (1.0-oRate) + n.y*rate;
    
    p.x += spd[i].x;
    p.y += spd[i].y;
   
#ifdef CALC_3D
    spd[i].z = spd[i].z * (1.0-oRate) + n.z*rate;
    p.z += spd[i].z;
#endif

    vbo.writePos(i, p);

}

void cApp::draw(){
    
    bOrtho ? mExp.beginOrtho() : mExp.beginPersp(58, 1, 100000); {

        gl::enableAlphaBlending();
        
        glPointSize(1);
        gl::clear( Colorf(0,0,0) );
        gl::pushMatrices();
        
        glTranslatef( mW/2-noiseCenter.x, mH/2-noiseCenter.y, 0);
        vbo.draw();
        gl::popMatrices();

    } mExp.end();
    
    mExp.draw();
    
    gl::color(0, 0, 0);
    gl::drawString( "fps : "+to_string(getAverageFps()) , Vec2i(20,20));
    gl::drawString( "frame : "+to_string(getElapsedFrames()) , Vec2i(20,40));
    
    frame++;
}

void cApp::keyDown( KeyEvent event ){
    switch (event.getChar()) {
        case 's':   mExp.snapShot();          break;
        case 'S':   mExp.startRender();       break;
        case 'T':   mExp.stopRender();        break;
        case 'f':   frame+=100;               break;
        case 'o':   bOrtho = !bOrtho;         break;
        case 'n':   bShowNoise = !bShowNoise; break;
    }
}

void cApp::mouseDown( MouseEvent event ){
    cout << event.getPos() << endl;
}

void cApp::mouseDrag( MouseEvent event ){
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
