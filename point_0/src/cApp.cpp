#define RENDER

#include "cinder/app/AppNative.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Camera.h"

#include "RfImporterBin.h"
#include "mtUtil.h"
#include "DataGroup.h"
#include "Exporter.h"
#include "TbbNpFinder.h"

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
    void mouseMove( MouseEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
#endif
    
    bool bOrtho = false;
     const int win_w = 1080*4;
    const int win_h = 1920;
    const float fps = 25.0f;
    int frame = 1000;
    int currentParam = 12;
    
    MayaCamUI camUi;
    CameraPersp camp;
    CameraOrtho camo;
    
    Exporter mExp, mExp2;
    DataGroup mDg;
    fs::path assetDir;
    fs::path simDir;
    
};

void cApp::setup(){
    
    int w = win_w;
    int h = win_h;
    
    //setFrameRate( fps );
    setWindowSize( w*0.25, h*0.25 );
    setWindowPos( 0, 0 );
    mExp.setup( w, h, 0, 500-1, GL_RGB, mt::getRenderPath()/"line", 0);
    //mExp2.setup( w, h, 0, 500-1, GL_RGB, mt::getRenderPath()/"point", 0);
    
    camp = CameraPersp( w, h, 54.4f, 1, 10000 );
    camp.lookAt( Vec3f(0,0, 1300), Vec3f(0,0,0) );
    camp.setCenterOfInterestPoint( Vec3f(0,0,0) );
    camUi.setCurrentCam( camp );
    
    // Ort
    camo = CameraOrtho( -w/2, w/2, h/2, -h/2, -10000, 10000 );

    assetDir = mt::getAssetPath();
    
#ifdef RENDER
    mExp.startRender();
    //mExp2.startRender();
#endif
}

void cApp::update(){
    
    RfImporterBin rfIn;

    char m[255];
    sprintf(m, "Binary_Loader01_%05d.bin", frame );
    string fileName = toString(m);
    simDir = assetDir/"sim"/"turb_r2"/"01";
    rfIn.load( (simDir/fileName).string() );
    
    mDg.mDot.reset();

    const vector<float> &p = rfIn.pPosition;

    vector<Vec3f> pos;
    vector<ColorAf> col;

    float scale = 60.0f * 1.6;
    
    scale *= bOrtho ? 1.5 : 1;
    
    //int loadnum = p.size() - frame*frame*0.05;
    
    int ef = getElapsedFrames();
    float rate  = 1.0f - (float)pow(ef, 5)/pow(500, 5);
    int loadnum = 64500 * rate;
    loadnum = MIN(loadnum, p.size()/3);

    cout << rate << ", " << loadnum << endl;
    
    for( int i=0; i<15; i++){
        pos.push_back( Vec3f(0,(i-7)*300, 0) );
        col.push_back( ColorAf(1,0,0,0.5) );
    }
    
    for (int i=0; i<p.size()/3; i++) {
        if( i>loadnum ) break;
        
        float x = p[i*3+0] * scale;
        float y = p[i*3+1] * scale;
        float z = p[i*3+2] * scale;
        pos.push_back( Vec3f(x, y, z) );

        ColorAf c;

        /*
         per particle data
         vector<float> pPosition;
         vector<float> pVelocity;
         vector<float> pForce;
         vector<float> pVorticity;
         vector<float> pNormal;
         vector<int>   pNumNeihbors;
         vector<float> pTexVec;
         vector<short> pInfoBit;
         vector<float> pElapsedTime;
         vector<float> pIsoTime;
         vector<float> pViscosity;
         
         vector<float> pDensity;
         vector<float> pPressure;
         vector<float> pMass;
         vector<float> pTemperature;
         vector<int> pId;
         */
        switch( currentParam ){
            case 0:
            {
                const vector<float> &v = rfIn.pVelocity;
                float vx = v[i*3+0];
                float vy = v[i*3+1];
                float vz = v[i*3+2];
                c.set(vx, vy, vz, 1);
                break;
            }
            case 1:
            {
                const vector<float> &v = rfIn.pForce;
                float vx = v[i*3+0];
                float vy = v[i*3+1];
                float vz = v[i*3+2];
                c.set(vx, vy, vz, 1);
                break;
            }
                
            case 2:
            {
                const vector<float> &v = rfIn.pVorticity;
                float vx = v[i*3+0];
                float vy = v[i*3+1];
                float vz = v[i*3+2];
                c.set(vx, vy, vz, 1);
                break;
            }
                
            case 3:
            {
                const vector<float> &v = rfIn.pNormal;
                float vx = v[i*3+0];
                float vy = v[i*3+1];
                float vz = v[i*3+2];
                c.set(vx, vy, vz, 1);
                break;
            }
                
            case 4:
            {
                const vector<int> &v = rfIn.pNumNeihbors;
                int vi = v[i];
                float vf = lmap((float)vi, 0.0f, 50.0f, 0.3f, 1.0f);
                c.set(1,1,1,vf);
                break;
            }
                
            case 5:
            {
                const vector<float> &v = rfIn.pTexVec;
                float vx = v[i*3+0];
                float vy = v[i*3+1];
                float vz = v[i*3+2];
                c.set(vx, vy, vz, 1);
                break;
            }
                
            case 6:
            {
                const vector<short> &v = rfIn.pInfoBit;
                short vs = v[i];
                float vf = lmap((float)vs, 0.0f, 255.0f, 0.3f, 1.0f);
                c.set(1,1,1,vf);
                break;
            }

            case 7:
            {
                const vector<float> &v = rfIn.pViscosity;
                float vf = v[i];
                vf = lmap(vf, 0.0f, 40.0f, 0.3f, 1.0f);
                c.set(1,1,1,vf);
                break;
            }
                
            case 8:
            {
                const vector<float> &v = rfIn.pDensity;
                float vf = v[i];
                vf = lmap(vf, 0.0f, 30.0f, 0.3f, 1.0f);
                c.set(1,1,1,vf);
                break;
            }

            case 9:
            {
                const vector<float> &v = rfIn.pPressure;
                float vf = v[i];
                vf = lmap(vf, 0.0f, 20.0f, 0.3f, 1.0f);
                c.set(1,1,1,vf);
                break;
            }

            case 10:
            {
                const vector<float> &v = rfIn.pTemperature;
                float vf = v[i];
                vf = lmap(vf, 0.0f, 255.0f, 0.3f, 1.0f);
                c.set(1,1,1,vf);
                break;
            }
                
            case 11:
            {
                const vector<int> &v = rfIn.pId;
                float vf = (float)v[i]/rfIn.gNumParticles;
                c.set(1,1,1,vf);
                
                if( v[i]<=300){
                    c.set(1, 0, 0, 1);
                }
                break;
            }
                
            case 12:
                const vector<float> &v1 = rfIn.pNormal;
                const vector<float> &v2 = rfIn.pViscosity;
                const vector<int> &v3 = rfIn.pNumNeihbors;
                const vector<float> &v4 = rfIn.pForce;

                
                float nx = v1[i*3+0]*0.5 + v4[i*3+0]*0.1;
                float ny = v1[i*3+1]*0.5 + v4[i*3+1]*0.1;
                float nz = v1[i*3+2]*0.5 + v4[i*3+2]*0.1;
                float vis = v2[i];
                int   nb = v3[i];
                float nbf = lmap((float)nb, 0.0f, 50.0f, 1.0f, 0.4f);
                
                vis = lmap(vis, 0.0f, 30.0f, 0.3f, 1.0f);
                
                c.set(nx, ny, nz+vis, 0.6+nbf);
                break;
                
        }
        
        col.push_back( c );
    }
    
    mDg.createDot( pos, col, 0.0 );
    
    
    int num_line = 2;
    int num_dupl = 1;
    int vertex_per_point = num_line * num_dupl * 2;
    vector<Vec3f> out;
    out.assign( pos.size()*vertex_per_point, Vec3f(-99999, -99999, -99999) );
    vector<ColorAf> outc;
    outc.assign( pos.size()*vertex_per_point, ColorAf(0,0,0,0) );
    
    TbbNpFinder npf;
    npf.findNearestPoints( &pos[0], &out[0], &col[0], &outc[0], pos.size(), num_line, num_dupl );
    mDg.addLine(out, outc);
}

void cApp::draw(){
    gl::enableAlphaBlending();
    
    //bOrtho ? mExp.begin( camo ) : mExp.begin( camp );
    mExp.begin( camUi.getCamera());
    {
        gl::clear();
        gl::pushMatrices();
        gl::rotate( Vec3f(90,0,0) );
        
        // draw rf
//        if( mDg.mLine ){
//            glLineWidth( 1 );
//            gl::draw( mDg.mLine );
//        }

        if( mDg.mDot ){
            glPointSize( 1 );
            gl::draw( mDg.mDot );
        }
        gl::popMatrices();
    }mExp.end();
    
    
//    mExp2.begin( camUi.getCamera());
//    {
//        gl::clear();
//        gl::pushMatrices();
//        gl::rotate( Vec3f(90,0,0) );
//
//        if( mDg.mDot ){
//            glPointSize( 1 );
//            gl::draw( mDg.mDot );
//        }
//        gl::popMatrices();
//        
//    }mExp2.end();

    gl::clear();
    mExp.draw();
    gl::drawString("Frame: " + to_string(frame), Vec2f(50, 50) );
    frame -= 2;

}

#ifndef RENDER
void cApp::keyDown( KeyEvent event ){

    switch (event.getChar()) {
        case 's':   mExp.snapShot();          break;
        case 'S':   mExp.startRender();       break;
        case 'T':   mExp.stopRender();        break;
        case 'f':   frame+=100;               break;
        case 'p':   currentParam++;           break;
        case 'q':   currentParam--;           break;
        case 'o':   bOrtho = !bOrtho;         break;
    }
}

void cApp::mouseDown( MouseEvent event ){
   camUi.mouseDown( event.getPos() );
}

void cApp::mouseMove( MouseEvent event ){
}

void cApp::mouseDrag( MouseEvent event ){
    camUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

#endif

CINDER_APP_NATIVE( cApp, RendererGl(0) )
