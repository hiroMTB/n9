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
    void loadColorSample( string fileName,  vector<Colorf> &col);
    
    const float master_scale = 0.5f;
    const int win_w = 4320;
    const int win_h = 1920;
    const float fps = 25.0f;
    int frame = 0;
    
    MayaCamUI camUi;
    Exporter mExp;
    Perlin mPln;
    Perlin mPln2;
    
    
    DataGroup mDgL;
    DataGroup mDgR;
    DataGroup mDgNearR;
    DataGroup mDgNearL;

    
    Vec3f pointL, pointR;

    vector<BSpline3f> mBSpsL;
    vector<BSpline3f> mBSpsR;
    
    vector<Colorf> mColorSample1;
    vector<Colorf> mColorSample2;

    vector<Vec3f> psL;
    vector<Vec3f> vsL;
    vector<ColorAf> csL;

    vector<Vec3f> psR;
    vector<Vec3f> vsR;
    vector<ColorAf> csR;

    bool mDir;
    
    gl::Texture guide;
};

void cApp::setup(){
    
    mDir = true;
    
    int w = win_w*master_scale;
    int h = win_h*master_scale;
    
    pointL.set( ( 160-win_w/2)*master_scale, (960-win_h/2)*master_scale, 0);
    pointR.set( (4160-win_w/2)*master_scale, (960-win_h/2)*master_scale, 0);
    
    setFrameRate( fps );
    setWindowSize( w, h );
    setWindowPos( 0, 0 );

    mExp.setup( w, h, 3000, GL_RGB, mt::getRenderPath(), 0);
    
    //CameraPersp cam;
    // CameraPersp cam( w, h, 60.0f, 1, 10000 );
    // cam.lookAt( Vec3f(0,0, 1300), Vec3f(0,0,0) );
    //camUi.setCurrentCam( cam );

    mPln.setSeed( 12345 );
    mPln.setOctaves( 3 );

    mPln2.setSeed( 33313 );
    mPln2.setOctaves( 3 );
    
    loadColorSample("img/Mx80_2_org_B.jpg", mColorSample1 );
    loadColorSample("img/Mx80_2_org_B.jpg", mColorSample2 );
    
    
    guide = loadImage( loadAsset("img/guide/laser_guide_01.png") );
    
    // mExp.startRender();
}

void cApp::loadColorSample( string fileName, vector<Colorf>& col){
    Surface sur( loadImage( loadAsset(fileName) ) );
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
    
    
    for( int i=0; i<psL.size(); i++ ){
        psL[i] += vsL[i];
        
        Vec3f adder = mPln.dfBm( vsL[i].x*0.1, vsL[i].y*0.1, vsL[i].z*0.1 );
        adder.x *= 0.3;
        vsL[i] = vsL[i]*0.999 + adder*0.001;
    }


    for( int i=0; i<psR.size(); i++ ){
        psR[i] += vsR[i];
        
        Vec3f adder = mPln.dfBm( vsR[i].x*0.1, vsR[i].y*0.1, vsR[i].z*0.1 );
        adder.x *= 0.3;
        vsR[i] = vsR[i]*0.999 + adder*0.001;
    }
    
    for (int i=0; i<3; i++) {
        ColorAf c = mColorSample1[i];
        c.a = 0.66;

        Vec2f n = mPln.dfBm(c.r, frame*0.01 );
        Vec3f vel = mPln2.dfBm(n.x, n.y, frame*0.05 );
        
        vel.x *= 0.1;
        vel.y *= 3;
        vel.z *= 1;
        
        psL.push_back( pointL );
        vsL.push_back(vel);
        csL.push_back( c );
    }
    
    for (int i=0; i<3; i++) {
        ColorAf c = mColorSample2[i];
        c.a = 0.66;
        Vec2f n = mPln.dfBm(c.g, 11+frame*0.01 );
        Vec3f vel = mPln.dfBm(n.x, n.y, 11+frame*0.01 );
        
        vel.x *= 0.1;
        vel.y *= 3;
        vel.z *= 1;
        
        psR.push_back( pointR );
        vsR.push_back( vel );
        csR.push_back( c );
        
    }
    
    vector<Vec3f> ctrlsL;
    for (int i=0; i<4; i++) {
        ctrlsL.push_back( pointL );
    }
    BSpline3f bpsL( ctrlsL, 3, false, true );
    mBSpsL.push_back(bpsL);
    
    vector<Vec3f> ctrlsR;
    for (int i=0; i<4; i++) {
        ctrlsR.push_back( pointR );
    }
    BSpline3f bpsR( ctrlsR, 3, false, true );
    mBSpsR.push_back(bpsR);


    mDgL.mDot.reset();
    mDgL.createDot( psL, csL, 0.0 );

    mDgR.mDot.reset();
    mDgR.createDot( psR, csR, 0.0 );

    
    // Spline
    for( int i=0; i< mBSpsL.size(); i++){
        Vec3f c0 = mBSpsL[i].getControlPoint(0);
        Vec3f newC0 = c0*0.99 + pointL*0.01;
        mBSpsL[i].setControlPoint(0, newC0 );
        for( int j=0; j<3; j++){
            mBSpsL[i].setControlPoint(j+1, psL[i*3+j]);
        }
    }

    for( int i=0; i< mBSpsR.size(); i++){
        Vec3f c0 = mBSpsR[i].getControlPoint(0);
        Vec3f newC0 = c0*0.99 + pointR*0.01;
        mBSpsR[i].setControlPoint(0, newC0 );

        for( int j=0; j<3; j++){
            mBSpsR[i].setControlPoint(j+1, psR[i*3+j]);
        }
    }
    
    // move
    if( mDir == true ){
        if( pointL.x>=(760-win_w/2)*master_scale ){
            mDir = false;
        }
    }else{
        if( pointL.x<(160-win_w/2)*master_scale ){
            mDir = true;
        }
    }
    pointL.x += 0.5f * (mDir?1:-1);
    pointR.x -= 0.5f * (mDir?1:-1);

    
    if(0){
        int num_line = 5;
        int num_dupl = 1;
        int vertex_per_point = num_line * num_dupl * 2;
        
        {
            
            vector<Vec3f> out;
            out.assign( psL.size()*vertex_per_point, Vec3f(-99999, -99999, -99999) );
            vector<ColorAf> outc;
            outc.assign( psL.size()*vertex_per_point, ColorAf(0,0,0,0) );
            
            TbbNpFinder npf;
            npf.findNearestPoints( &psL[0], &out[0], &csL[0], &outc[0], psL.size(), num_line, num_dupl );
            mDgNearL.addLine(out, outc);
        }
        
        {
            vector<Vec3f> out;
            out.assign( psR.size()*vertex_per_point, Vec3f(-99999, -99999, -99999) );
            vector<ColorAf> outc;
            outc.assign( psR.size()*vertex_per_point, ColorAf(0,0,0,0) );
            
            TbbNpFinder npf;
            npf.findNearestPoints( &psR[0], &out[0], &csR[0], &outc[0], psR.size(), num_line, num_dupl );
            mDgNearR.addLine(out, outc);
        }
    }
}

void cApp::draw(){
    
    //mExp.begin( camUi.getCamera() );{
    mExp.begin();{
    
        gl::clear();
        gl::enableAlphaBlending();
        gl::pushMatrices();
        gl::setMatricesWindowPersp(getWindowWidth(), getWindowHeight(), 60.0f, 1.0f, 100000.0f );
        
        if( !mExp.bRender && !mExp.bSnap ){
            mt::drawCoordinate(100);
//            glPushMatrix();
//            gl::color( Color::white() );
//            gl::scale(0.5, 0.5);
//            gl::draw( guide );
//            glPopMatrix();
        }

        gl::translate( win_w*master_scale/2, win_h*master_scale/2 );

        // draw
        if( mDgNearL.mLine ){
            glLineWidth( 1 );
            gl::draw( mDgNearL.mLine );
        }
        
        if( mDgNearR.mLine ){
            glLineWidth( 1 );
            gl::draw( mDgNearR.mLine );
        }
        
        if( mDgL.mLine ){
            glLineWidth( 1 );
            gl::draw( mDgL.mLine );
        }
        
        if( mDgR.mLine ){
            glLineWidth( 1 );
            gl::draw( mDgR.mLine );
        }
        


        // draw spline
        int resolution = 30;
        glLineWidth(1);
        for( int i=0; i<mBSpsL.size(); i++ ){
            ColorAf c = mColorSample2[i*10];
            c.a = 0.25;
            glColor4f(c);
            glBegin(GL_LINES);
            for( int j=0; j<resolution-1; j++ ){
                float rate1 = (float)j/resolution;
                float rate2 = (float)(j+1)/resolution;
                Vec3f v1 = mBSpsL[i].getPosition( rate1 );
                Vec3f v2 = mBSpsL[i].getPosition( rate2 );
                glVertex3f( v1 );
                glVertex3f( v2 );
            }
            glEnd();
        }
        
        for( int i=0; i<mBSpsR.size(); i++ ){
            
            ColorAf c = mColorSample1[i*20];
            c.a = 0.25;
            glColor4f(c);
            
            glBegin(GL_LINES);
            for( int j=0; j<resolution-1; j++ ){
                float rate1 = (float)j/resolution;
                float rate2 = (float)(j+1)/resolution;
                Vec3f v1 = mBSpsR[i].getPosition( rate1 );
                Vec3f v2 = mBSpsR[i].getPosition( rate2 );
                glVertex3f( v1 );
                glVertex3f( v2 );
            }
            glEnd();
        }
        
        
        
        if( mDgL.mDot ){
            glPointSize( 1 );
            gl::draw( mDgL.mDot );
        }
        
        if( mDgR.mDot ){
            glPointSize( 1 );
            gl::draw( mDgR.mDot );
        }
        
        
        {
            glColor3f(1, 0, 0);
            glPointSize(3);
            glBegin(GL_POINTS);
            glVertex3f(pointL);
            glVertex3f(pointR);
            glEnd();
        }
        
        gl::popMatrices();
        
    }mExp.end();

    gl::clear( ColorA(1,1,1,1) );
    gl::color( Color::white() );
    mExp.draw();
    
    frame++;

}

void cApp::keyDown( KeyEvent event ){

    switch (event.getChar()) {
        
        case 'R':
            mExp.startRender();
            break;

        case 'T':
            mExp.stopRender();
            break;

        case 'f':
            frame+=500;
            break;
            
        default:
            break;
    }
}

void cApp::mouseDown( MouseEvent event ){
   //camUi.mouseDown( event.getPos() );
}

void cApp::mouseMove( MouseEvent event ){
}

void cApp::mouseDrag( MouseEvent event ){
    //camUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void cApp::resize(){
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
