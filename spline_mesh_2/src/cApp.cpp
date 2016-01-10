//#define RENDER

#include "cinder/app/AppNative.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Perlin.h"
#include "cinder/params/Params.h"
#include "CinderOpenCv.h"

#include "mtUtil.h"
#include "Exporter.h"
#include "MyCamera.h"
#include "ContourMap.h"

#include <boost/format.hpp>

using boost::format;
using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void keyDown( KeyEvent event );
    void update();
    void draw();
    void drawSpline();
    void makeSpline();
    void makePMesh();
    void makeLMesh();
    
    int mWin_w = 1080*4;
    int mWin_h = 1920;
    float winScale = 0.6;
    float surW;
    float surH;
    float extrusion = 200;
    
    vector<CameraPersp> cams;
    Exporter mExp;
    
    Surface32f sur1;
    Surface32f sur2;

    vector<vector<Colorf>> mColorSample1;
    vector<vector<Colorf>> mColorSample2;
    
    gl::VboMeshRef pmesh;
    gl::VboMeshRef lmesh;
    
    vector<vector<Vec3f>> pss;
    vector<vector<ColorAf>> css;
    vector<BSpline<Vec3f>> sps;
    
    int pmesh_res_x = 3;
    int pmesh_res_y = 3;
    int lmesh_res_x = 1;
    int lmesh_res_y = 512/2;
    int sp_res_x = 3;
    int sp_res_y = 256;
    
    float frame = 0;;
    float duration = 25;   // A -> B blend num
    
};

void cApp::setup(){
    
    setFrameRate( 25 );
    setWindowPos( 0, 0 );
    setWindowSize( mWin_w*winScale, mWin_h*winScale);
    mExp.setup( mWin_w, mWin_h, 500, GL_RGB, mt::getRenderPath(), 0 );
    
    CameraPersp cam1( mWin_w, mWin_h, 30, 1.0f, 10000.0f );
    //cam1.lookAt( Vec3f(0,0,2200), Vec3f(0,0,0) );
    cam1.lookAt( Vec3f(0,0,6000), Vec3f(0,0,0) );
    cams.push_back(cam1);

    fs::path assetDir = mt::getAssetPath();
    
    //mt::loadColorSample("img/geo/earth/earth_8.png", mColorSample1 );
    //mt::loadColorSample("img/geo/earth/earth_1.png", mColorSample2 );
    mt::loadColorSample("img/colorSample/n5pickCol_2.png", mColorSample1 );
    mt::loadColorSample("img/colorSample/n5pickCol_3.png", mColorSample2 );
    
    // load image
    //string file = "img/geo/landsat/LC80770172015167LGN00/s/LC80770172015167LGN00_B6_s.TIF";
    //string file = "img/geo/landsat/LC81450312015292LGN00/s/LC81450312015292LGN00_B4_s.tif";
    //string file = "img/geo/landsat/LC81640112014182LGN00/s/LC81640112014182LGN00_B4_s.tif";
    
    string file1 = "img/geo/alos/N046E007/AVERAGE/N046E007_AVE_DSM.tif";
    string file2 = "img/geo/alos/N031E107/AVERAGE/N031E107_AVE_DSM.tif";
    
    sur1 = Surface32f( loadImage(assetDir/file1) );
    sur2 = Surface32f( loadImage(assetDir/file2) );
    surW = sur1.getWidth();
    surH = sur1.getHeight();
    
#ifdef RENDER
    mExp.startRender();
#endif
    
}

void cApp::makePMesh() {
    
    if( pmesh ) pmesh->reset();
    
    int loadW = surW/pmesh_res_x;
    int loadH = surH/pmesh_res_y;
    int nVertices = loadW * loadH;
    pmesh = gl::VboMesh::create( nVertices, 0, mt::getVboLayout(), GL_POINTS );
    gl::VboMesh::VertexIter vitr = pmesh->mapVertexBuffer();
    
    boost::format fmt( "Make Point VboMesh %d resolution_x, %d resolution_y, %d vertices");
    fmt % pmesh_res_x % pmesh_res_y % nVertices;
    cout << fmt.str() << endl;
    
    for( int i=0; i<loadH; i++){
        for( int j=0; j<loadW; j++){
            
            float idx = j*pmesh_res_x;
            float idy = i*pmesh_res_y;
            Vec3f  p = pss[idx][idy];
            p.z = log10(p.z) * log10(p.z) * 150;
            ColorAf c = css[idx][idy];
            c.a = 0.5;
            vitr.setPosition( p );
            vitr.setColorRGBA( c );
            ++vitr;
        }
    }
}

void cApp::makeLMesh(){

    if(lmesh)lmesh->reset();
    
    int surWx2 = (int)surW - (int)surW%2;
    int surHx2 = (int)surH - (int)surH%2;

    int loadW = surWx2/lmesh_res_x;
    int loadH = surHx2/lmesh_res_y;
    int nVertices = loadW * loadH;
    lmesh = gl::VboMesh::create( nVertices*2, 0, mt::getVboLayout(), GL_LINES );
    gl::VboMesh::VertexIter vitr = lmesh->mapVertexBuffer();
    
    boost::format fmt( "Make Line VboMesh %d resolution_x, %d resolution_y, %d vertices");
    fmt % lmesh_res_x % lmesh_res_y % nVertices;
    cout << fmt.str() << endl;
    
    for( int i=0; i<loadH-1; i++){
        for( int j=0; j<loadW-1; j++){
            
            float idx = j*lmesh_res_x;
            float idy = i*lmesh_res_y;
            float idx2 = (j+1)*lmesh_res_x;
            Vec3f p1 = pss[idx][idy];
            Vec3f p2 = pss[idx2][idy];
            p1.z = -log10(p1.z) * 200;
            p2.z = -log10(p2.z) * 200;
            
            ColorAf c1 = css[idx][idy];
            ColorAf c2 = css[idx2][idy];
            c1.r *= 1.5;
            c2.r *= 1.5;
            c1.a  = 0.75;
            c2.a = 0.75;
            
            vitr.setPosition( p1 );
            vitr.setColorRGBA( c1 );
            ++vitr;
            
            vitr.setPosition( p2 );
            vitr.setColorRGBA( c2 );
            ++vitr;
        }
    }
}

void cApp::makeSpline(){
    
    sps.clear();
    
    int loadW = surW/sp_res_x;
    int loadH = surH/sp_res_y;

    for(int i=0; i<loadH; i++){
        vector<Vec3f> ctrl;
        for(int j=0; j<loadW; j++){
            float idx = j*sp_res_x;
            float idy = i*sp_res_y;
            Vec3f p = pss[idx][idy];
            p.z = log10(p.z)*log10(p.z) * 100;
            ctrl.push_back( p );
        }
        
        BSpline<Vec3f> spline(ctrl, 6, false, false);
        sps.push_back( spline );
    }
}

void cApp::update(){
    
    
    for (auto e: pss) e.clear();
    for (auto e: css) e.clear();
    
    pss.clear();
    css.clear();
    
    float rateB = frame/duration;
    float rateA = 1.0f - rateB;
    
    for( int i=0; i<surW; i++){
        vector<Vec3f> ps;
        vector<ColorAf> cs;
        
        for( int j=0; j<surH; j++){
            
            float x = i - surW/2;
            float y = j - surH/2;
            ColorAf colA = sur1.getPixel(Vec2i(j, i) );
            ColorAf colB = sur2.getPixel(Vec2i(j, i) );

            float lumA = (colA.r+colA.g+colA.b) * 0.3333;
            float lumB = (colB.r+colB.g+colB.b) * 0.3333;
            float lumMix = lumA*rateA + lumB*rateB;
            float z = lumMix;
            
            Colorf & col1 = mColorSample1[i%mColorSample1.size()][j%mColorSample1[0].size()];
            Colorf & col2 = mColorSample2[i%mColorSample2.size()][j%mColorSample2[0].size()];
            Colorf colMix = col1*rateA + col2*rateB;
            cs.push_back( ColorAf(colMix.r, colMix.g, colMix.b, 0.7) );
            
            ps.push_back( Vec3f( x, y, z) );
        }
        
        pss.push_back( ps );
        css.push_back( cs );
    }
    
    
    makePMesh();
    makeLMesh();
    makeSpline();
    
    
    frame++;
}

void cApp::drawSpline(){
    int spline_draw_res = 2000;
    for(int i=0; i<sps.size(); i++){
        
        const BSpline<Vec3f> & bsp = sps[i];
        glBegin( GL_LINE_STRIP );
        //glColor4f( css[i][0] );
        
        for(int j=0; j<spline_draw_res; j++ ){
            Vec3f v = bsp.getPosition((float)j/spline_draw_res);
            ColorAf c = mColorSample2[i%mColorSample2.size()][j%mColorSample2[0].size()];
            glColor4f(1.0-i*0.1, c.r, 2.0+i*0.1 , 0.75 );
            glVertex3f(v);
        }
        gl::end();
    }
}

void cApp::draw(){
    
    gl::clear();
    gl::enableAlphaBlending();
    
    mExp.begin(cams[0]); {
        gl::clear();
        gl::rotate( Vec3f( 90, 0, 0 ));
        glTranslatef(0,0,-500);
        glPointSize( 1 );
        glLineWidth( 1 );
        
        if(pmesh)gl::draw( pmesh );

        gl::enableAdditiveBlending();
        if(lmesh)gl::draw( lmesh );
        drawSpline();
        
    } mExp.end();
    
    
    gl::color( Colorf::white() );
    mExp.draw();
}
 

void cApp::keyDown( KeyEvent event ) {
    char key = event.getChar();
    switch (key) {
        case 'S':
            mExp.snapShot();
            break;
        case 'T':
            mExp.stopRender();
            break;
        }
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
