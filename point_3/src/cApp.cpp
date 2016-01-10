//#define RENDER

#include "cinder/app/AppNative.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Camera.h"

#include "RfImporterBin.h"
#include "RfImporterRpc.h"

#include "mtUtil.h"
#include "DataGroup.h"
#include "Exporter.h"
#include "TbbNpFinder.h"
#include "VboSet.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
public:
    void setup();
    void update();
    void draw();

#ifndef RENDER
    void keyDown( KeyEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
#endif
    
    const float master_scale = 1;
    const int win_w = 1080*4;
    const int win_h = 1920;
    const float fps = 25.0f;
    int frame = 0;
    
    MayaCamUI camUi;
    Exporter mExp;
    fs::path assetDir;
    vector<fs::path> simDir;
    
    bool bStart = true;
    
    vector<VboSet> vbo;
};

void cApp::setup(){
    
    int w = win_w*master_scale;
    int h = win_h*master_scale;
    
    setFrameRate( fps );
    setWindowSize( w*0.5, h*0.5 );
    setWindowPos( 0, 0 );
    mExp.setup( w, h, 3000, GL_RGB, mt::getRenderPath(), 0);
    
    CameraPersp cam( w, h, 54.4f, 1, 10000 );
    cam.lookAt( Vec3f(0,0, 1300), Vec3f(0,0,0) );
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    camUi.setCurrentCam( cam );

    assetDir = mt::getAssetPath();
    simDir.push_back( assetDir/"sim"/"red_particle3"/"rpc2");
    
    int nSim = simDir.size();
    vbo.assign( nSim, VboSet() );
    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){
    
    if( !bStart ) return;
    
    //RfImporterBin rfIn;
    RfImporterRpc rfIn;

    char m[255];
    //sprintf(m, "Binary_Loader01_%05d.bin", frame );
    sprintf(m, "_Domain_%05d.rpc", frame%1000);
    //sprintf(m, "HY_Closed_Domain_Domain_%05d.rpc", frame%100);
    string fileName = toString(m);
    printf("    %s\n", fileName.c_str());
    
    int nSim = simDir.size();
    for( int i=0; i<nSim; i++ ){

        rfIn.load( (simDir[i]/fileName).string() );

        VboSet & vb = vbo[i];
        vb.resetCol();
        vb.resetPos();
        
        const vector<float> &p = rfIn.pPosition;
        
        float scale = 50.0f;
        
        for (int i=0; i<p.size()/3; i++) {

            float x = p[i*3+0] * scale;
            float y = p[i*3+1] * scale;
            float z = p[i*3+2] * scale;
            vb.addPos( Vec3f(x, y, z) );
            
            ColorAf c(1,0,0,1);
            vb.addCol( c );
        }
        
        vb.init(true, true, true, GL_POINTS);
    }
}

void cApp::draw(){
    mExp.begin( camUi.getCamera() );{
        
        gl::clear();
        gl::enableAlphaBlending();
        gl::pushMatrices();

        glPointSize( 1 );
        for( auto & v : vbo){
            v.draw();
        }
        gl::popMatrices();
        
    }mExp.end();

    gl::clear( ColorA(1,1,1,1) );
    gl::color( Color::white() );
    mExp.draw();
    
    gl::drawString("Frame: " + to_string(frame), Vec2f(50, 50) );
    frame++;
}

#ifndef RENDER
void cApp::keyDown( KeyEvent event ){

    switch (event.getChar()) {
        case 's':   mExp.snapShot();          break;
        case 'S':   mExp.startRender();       break;
        case 'T':   mExp.stopRender();        break;
        case 'f':   frame+=100;               break;
        case ' ':   bStart = !bStart;         break;
        
    }
}

void cApp::mouseDown( MouseEvent event ){
   camUi.mouseDown( event.getPos() );
}

void cApp::mouseDrag( MouseEvent event ){
    camUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

#endif

CINDER_APP_NATIVE( cApp, RendererGl(0) )
