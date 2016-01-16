//#define RENDER
//#define PROXY

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
#include "RfImporterBin.h"

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
    
    bool bStart = false;
    bool bOrtho = false;
    int  mW;
    int  mH;
    float dispScale = 0.5;
    double frame    = 100;
    const vector<Vec2i> absPos = { Vec2i(1640,192), Vec2i(2680,192), Vec2i(1640,1728), Vec2i(2680,1728) };
    fs::path assetDir = mt::getAssetPath();
    vector<VboSet> vbos;
    Exporter mExp;
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
    
    vbos.assign(2, VboSet() );
    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){
    if(!bStart) return;
    
    for(int k=0; k<vbos.size(); k++ ){
        VboSet & vbo = vbos[k];
        vbo.resetVbo();
        
        stringstream fileName;
        fileName << "red2_" << std::setw(5) << setfill('0') << frame << ".bin";
        printf( "start loading RFbin : %s\n", fileName.str().c_str() );
        
        RfImporterBin rfb;
        rfb.load( (assetDir/"sim"/"red_particle_pln1"/("set"+to_string(k))/fileName.str()).string() );
        
        const vector<float> & pos = rfb.pPosition;
        float rfScale = 1.0/0.05;
        
        int loadnum = pos.size()/3 * (1.0-frame/1000.0);
        for( int i=0; i<loadnum; i++){
            vbo.addPos( Vec3f(pos[i*3+0], pos[i*3+1],pos[i*3+2]) * rfScale );
            vbo.addCol( ColorAf(1,0,0,1) );
        }
        
        if(!vbo.vbo){
            vbo.init( GL_POINTS );
        }else{
            vbo.updateVboPos();
            vbo.updateVboCol();
        }
    }
}

void cApp::draw(){
    
    bOrtho ? mExp.beginOrtho() : mExp.beginPersp(58, 1, 100000); {

        gl::enableAlphaBlending();
        
        glPointSize(1);
        gl::clear( Colorf(0,0,0) );
        gl::pushMatrices();
        
        glTranslatef( -mW/2, -mH/2, 0);
        
        for( int i=0;i<vbos.size(); i++){
            gl::pushMatrices();
            glTranslatef( absPos[i].x, absPos[i].y, 0);
            vbos[i].draw();
            gl::popMatrices();
        }

        gl::popMatrices();

    } mExp.end();
    
    mExp.draw();
    
    glColor3f(0, 1, 0);
    for( int i=0;i<absPos.size(); i++){
        gl::drawStrokedCircle(Vec2i(absPos[i].x*dispScale, absPos[i].y*dispScale), 5);
    }

    gl::color(0, 0, 0);
    gl::drawString( "fps : "+to_string(getAverageFps()) , Vec2i(20,20));
    gl::drawString( "frame : "+to_string(getElapsedFrames()) , Vec2i(20,40));
    //gl::drawString( "nParticle : "+to_string(vbo[].getPos().size()) , Vec2i(20,60));
    
    if(bStart)  
        frame++;
}

void cApp::keyDown( KeyEvent event ){
    switch (event.getChar()) {
        case 's':   mExp.snapShot();          break;
        case 'S':   mExp.startRender();       break;
        case 'T':   mExp.stopRender();        break;
        case 'f':   frame+=100;               break;
        case 'o':   bOrtho = !bOrtho;         break;
        case ' ':   bStart = !bStart;         break;
    }
}

void cApp::mouseDown( MouseEvent event ){
    cout << event.getPos() << endl;
}

void cApp::mouseDrag( MouseEvent event ){
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
