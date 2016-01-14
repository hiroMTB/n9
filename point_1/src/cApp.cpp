//#define RENDER
#include "cinder/app/AppNative.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Camera.h"
#include "cinder/Perlin.h"

#include "mtUtil.h"
#include "RfImporterBin.h"
#include "DataGroup.h"
#include "Exporter.h"
#include "TbbNpFinder.h"
#include <boost/format.hpp>

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
    void loadColorSample( fs::path filepath,  vector<Colorf> &col);
    
    const float master_scale = 1;
    const int win_w = 1080*2;
    const int win_h = 1920;
    const float fps = 25.0f;
    int frame = 1;
    const int endframe = 501;
    vector<BSpline2f> mBSps;
    vector<Colorf> mColorSample1;
    vector<Colorf> mColorSample2;
    
    MayaCamUI camUi;
    Exporter mExp, mExp2;
    DataGroup mDg;
    
    fs::path assetDir;
    fs::path renderDir;
    vector<fs::path> simDirList;
    
    int simDirNum = 0;
    
    vector<CameraPersp> cams;
    
    int numParticle = 0;
    static const bool bDrawLine = false;
};

void cApp::setup(){
    
    int sub = 0;
    for( int i=0; i<=6; i++){
        
        sub+= pow(2, i);
    }
    
    int w = win_w*master_scale;
    int h = win_h*master_scale;
    
    setFrameRate( fps );
    setWindowSize( w*0.5, h*0.5 );
    setWindowPos( 0, 0 );
    renderDir = mt::getRenderPath();
    mExp.setup( w, h, 0, 3000, GL_RGB, renderDir/to_string(simDirNum), 0);
    
    if( bDrawLine ) mExp2.setup( w, h, 0, 3000, GL_RGB, mt::getRenderPath()/"line"/to_string(simDirNum), 0);
    
    CameraPersp top( w, h, 60, 1, 1000000 );
    top.lookAt( Vec3f(0,0, 1300), Vec3f(0,0,0) );
    top.setCenterOfInterestPoint( Vec3f(0,0,0) );
    
    CameraPersp side( w, h, 60, 1, 1000000 );
    side.lookAt( Vec3f(0, 1300, 0), Vec3f(0,0,0) );
    side.setLensShift(0, 0);
    side.setCenterOfInterestPoint( Vec3f(0,0,0) );

    CameraPersp side2( w, h, 60, 1, 1000000 );
    side2.lookAt( Vec3f(1300, 0, 0), Vec3f(0,0,0) );
    side2.setLensShift(0, 0);
    side2.setCenterOfInterestPoint( Vec3f(0,0,0) );

    CameraPersp diag( w, h, 60, 1, 1000000 );
    diag.lookAt( Vec3f(1300/sqrt(2), 1300/sqrt(2), 0), Vec3f(0,0,0) );
    diag.setLensShift(0, 0);
    diag.setCenterOfInterestPoint( Vec3f(0,0,0) );

    CameraPersp diag2( w, h, 60, 1, 1000000 );
    diag2.lookAt( Vec3f(0, 1300/sqrt(2), 1300/sqrt(2)), Vec3f(0,0,0) );
    diag2.setLensShift(0, 0);
    diag2.setCenterOfInterestPoint( Vec3f(0,0,0) );

    cams.push_back(top);
    cams.push_back(side);
    cams.push_back(side2);
    cams.push_back(diag);
    cams.push_back(diag2);

    camUi.setCurrentCam( top );

    assetDir = mt::getAssetPath();
    for( int i=0; i<40; i++){
        //boost::format fmt("Circle01_%02d"); fmt % i;
        //fs::path simDir = assetDir/"sim"/"smooth_r3"/fmt.str();
        
        fs::path simDir = assetDir/"sim"/"absorb_r1";
        
        simDirList.push_back( simDir );
    }

    
    loadColorSample(assetDir/"img"/"colorSample"/"n5pickCol_0.png", mColorSample1 );
    if( bDrawLine ) loadColorSample(assetDir/"img"/"colorSample"/"n5pickCol_1.png", mColorSample2 );
    
#ifdef RENDER
    mExp.startRender();
    if( bDrawLine )mExp2.startRender();
#endif
}

void cApp::loadColorSample( fs::path filepath, vector<Colorf>& col){
    Surface sur( loadImage( filepath ) );
    Surface::Iter itr = sur.getIter();
    
    while ( itr.line() ) {
        while ( itr.pixel() ) {
            float r = itr.r() / 255.0f;
            float g = itr.g() / 255.0f;
            float b = itr.b() / 255.0f;
            col.push_back( Colorf(r,g,b) );
        }
    }
    cout << "Load Success: " << col.size() << " ColorSample" << endl;
    
}

void cApp::update(){
    
    if( frame == endframe ){
        frame = 1;
        simDirNum++;
        if( 40<=simDirNum ){
            printf("DONE\n");
            quit();
        }else{
            printf(" --- DONE\n");
            printf("start rendering : simDirNum = %02d ", simDirNum );

            int w = win_w*master_scale;
            int h = win_h*master_scale;
            mExp.clear();
            mExp.setup( w, h, 0, 3000, GL_RGB, renderDir/to_string(simDirNum), 0);
#ifdef RENDER
            mExp.startRender();
#endif
        }
    }
    fs::path & simDir = simDirList[simDirNum];
    
    RfImporterBin rfIn;

    char m[255];
    //sprintf(m, "Circle01_%05d.bin", frame );
    sprintf(m, "Binary_Loader02_%05d.bin", frame );
    
    string fileName = toString(m);

    fs::path simFilePath = simDir / fileName;
    rfIn.load( simFilePath.string() );
    
    mDg.mDot.reset();

    numParticle = rfIn.gNumParticles;
    const vector<float> &p = rfIn.pPosition;
    const vector<float> &v = rfIn.pVelocity;
    const vector<int> &pid = rfIn.pId;
    
    vector<Vec3f> pos;
    vector<ColorAf> col;
    vector<ColorAf> col4Line;

    float scale = 55.0f;
    
    for (int i=0; i<numParticle; i++) {
        
        float x = p[i*3+0] * scale;
        float y = p[i*3+2] * scale;
        float z = p[i*3+1] * scale;
        pos.push_back( Vec3f(x, y, z) );
        
        float vx = v[i*3+0];
        float vy = v[i*3+2];
        float vz = v[i*3+1];
        float vel = sqrt(vx*vx + vy*vy + vz*vz);
        float velRate = lmap(vel, 0.03f, 15.0f, 0.0f, 1.0f);
        int colorId = pid[i] % mColorSample1.size();

        ColorAf c = mColorSample1[colorId];
        c.r += 0.2;
        c.g += 0.2;
        c.b += 0.2;
        
        c.g += velRate*0.1;
        c.a = 0.95;
        
        col.push_back( c );

        if( bDrawLine ){
            ColorAf c2 = mColorSample2[colorId];
            c2.a = lmap(vel, 0.0f, 1.0f, 0.1f, 0.6f);
            col4Line.push_back( c2 );
        }
    }
    
    mDg.createDot( pos, col, 0.0 );

    if( bDrawLine ){
        int num_line = 1;//randFloat(1,3);
        int num_dupl = 1;//randFloat(1,3);
        int vertex_per_point = num_line * num_dupl * 2;
        vector<Vec3f> out;
        out.assign( pos.size()*vertex_per_point, Vec3f(-99999, -99999, -99999) );
        vector<ColorAf> outc;
        outc.assign( pos.size()*vertex_per_point, ColorAf(0,0,0,0) );
        
        TbbNpFinder npf;
        npf.findNearestPoints( &pos[0], &out[0], &col4Line[0], &outc[0], pos.size(), num_line, num_dupl );
        mDg.addLine(out, outc);
    }
}

void cApp::draw(){
    mExp.begin( camUi.getCamera() );{
        gl::clear();
        gl::enableAlphaBlending();
        gl::pushMatrices();
        
        if( !mExp.bRender && !mExp.bSnap ){
            glLineWidth( 3 );
            mt::drawCoordinate(100);
        }
        if( mDg.mDot ){
            glPointSize( 1 );
            gl::draw( mDg.mDot );
        }
        
        gl::popMatrices();
        
    }mExp.end();

    if( bDrawLine ){
        mExp2.begin( camUi.getCamera() );{
            gl::clear();
            gl::enableAlphaBlending();
            gl::pushMatrices();
            if( mDg.mLine ){
                glLineWidth( 1 );
                gl::draw( mDg.mLine );
            }
            gl::popMatrices();
        }mExp2.end();
    }
    
    //gl::enableAdditiveBlending();
    gl::clear( ColorA(0,0,0,1) );
    gl::color( Color::white() );
    mExp.draw();
    if( bDrawLine )
        mExp2.draw();

    gl::drawString("Frame: " + to_string(frame), Vec2f(50, 50) );
    gl::drawString("simDirNum: " + to_string(simDirNum), Vec2f(50, 75) );
    gl::drawString("numParticle: " + to_string(numParticle), Vec2f(50, 100) );
    
    
#ifdef RENDER
    frame+=1;
#else
    frame+=1;
#endif

}

#ifndef RENDER
void cApp::keyDown( KeyEvent event ){
    switch (event.getChar()) {
        case '1': camUi.setCurrentCam(cams[0]);     break;
        case '2': camUi.setCurrentCam(cams[1]);     break;
        case '3': camUi.setCurrentCam(cams[2]);     break;
        case '4': camUi.setCurrentCam(cams[3]);     break;
        case '5': camUi.setCurrentCam(cams[4]);     break;
        case 'R': mExp.startRender();               break;
        case 'T': mExp.stopRender();                break;
        case 'f': frame+=100;                       break;
        case '0': frame=0; simDirNum++;             break;
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
