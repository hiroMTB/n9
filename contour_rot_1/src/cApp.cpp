#define RENDER

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

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void keyDown( KeyEvent event );
    void update();
    void draw();
    
    
    int mWin_w = 1080*4;
    int mWin_h = 1920;
    int frameL = 0;
    int frameR = 0;
    
    gl::Texture mTex;
    params::InterfaceGlRef mParams;

    ContourMap cmL, cmR;
    Exporter mExp;

    Vec2f scanPointL, scanPointR;

    vector<Vec2f> allpointsL, allpointsR;
    
    vector<vector<Colorf>> mColorSample1;
    vector<vector<Colorf>> mColorSample2;
};

void cApp::setup(){
    
    setFrameRate( 25 );
    
    setWindowPos( 0, 0 );
    setWindowSize( mWin_w*0.5, mWin_h*0.5 );
    mExp.setup( mWin_w, mWin_h, 3001, GL_RGB, mt::getRenderPath(), 0 );

    fs::path assetDir = mt::getAssetPath();
    
    mt::loadColorSample("img/geo/earth/earth_8.png", mColorSample1 );
    mt::loadColorSample("img/geo/earth/earth_1.png", mColorSample2 );
    
    // load image
    Surface32f surL( loadImage(assetDir/("img/geo/alos/N031E107/AVERAGE/N031E107_AVE_DSM.tif")));
    Surface32f surR( loadImage(assetDir/("img/geo/alos/N046E007/AVERAGE/N046E007_AVE_DSM.tif")));

    cmL.setImage( surL, true, cv::Size(8,8) );
    cmR.setImage( surR, true, cv::Size(8,8) );

    cmL.addContour( 0.0085, 0);
    cmR.addContour( 0.02, 0);
    cmR.addContour( 0.03, 0);

    surL.reset();
    surR.reset();
    
#ifdef RENDER
    mExp.startRender();
#endif
    
    {
        // Prepare L
        float railWidth = 1200; // 1200 pix = 5000m
        float imgWidth = cmL.width;
        float scale = railWidth/imgWidth;
        
        scanPointL.set( 0,0 );
        int offsetx = cmL.width/2 * scale;
        for( int i=0; i<cmL.mCMapData.size(); i++ ){
            
            ContourMap::ContourGroup &cg = cmL.mCMapData[i];
            for( int j=0; j<cg.size(); j++ ){
                
                ContourMap::Contour & c = cg[j];
                
                if( c.size() > 100 ){
                    for( int k=c.size()-1; k!=-1; k-- ){
                        cv::Point2f & p = c[k];
                        Vec2f tpos = fromOcv(p);
                        tpos *= scale;
                        tpos.x -= offsetx;
                        allpointsL.push_back( tpos );
                    }
                }
            }
        }
    }
    
    {
        // Prepare R
        float railWidth = 1200; // 1200 pix = 5000m
        float imgWidth = cmR.width;
        float scale = railWidth/imgWidth;
        
        scanPointR.set( 0,0 );
        int offsetx = cmR.width/2 * scale;
        for( int i=0; i<cmR.mCMapData.size(); i++ ){
            
            ContourMap::ContourGroup &cg = cmR.mCMapData[i];
            for( int j=0; j<cg.size(); j++ ){
                
                ContourMap::Contour & c = cg[j];
                
                if( c.size() > 100 ){
                    for( int k=c.size()-1; k!=-1; k-- ){
                        cv::Point2f & p = c[k];
                        Vec2f tpos = fromOcv(p);
                        tpos *= scale;
                        tpos.x -= offsetx;
                        allpointsR.push_back( tpos );
                    }
                }
            }
        }
    }
}

void cApp::update(){
}

void cApp::draw(){
    
    gl::clear();
    gl::enableAlphaBlending();
    mExp.begin(); {
        gl::clear();
        
        {
            float scanSpeed = 48;
            int head = frameL * scanSpeed;
            int next_head = (frameL+1) * scanSpeed;
            if( next_head >= allpointsL.size() ){
                quit();
            }
            
            Vec2f newScanPoint = allpointsL[next_head];
            Vec2f dir = newScanPoint - scanPointL;
            float dist = dir.length();
            if( scanSpeed/4 <= dist && dist< scanSpeed ){
                scanPointL = newScanPoint;
            }else if( scanSpeed <= dir.length()  ){
                scanPointL += dir.normalized()*scanSpeed;
            }else{
                scanPointL = newScanPoint;
                frameL++;
            }
            
            // LEFT contour
            gl::pushMatrices();{
                glTranslatef( 600+120, mExp.mFbo.getHeight()/2, 0);
                
                glPointSize(1);
                glBegin( GL_POINTS );
                for( int i=0; i<head; i++ ){
                    Vec2f & tpos = allpointsL[i];
                    int x = (int)abs(tpos.x) % mColorSample1.size();
                    int y = (int)abs(tpos.y) % mColorSample1[0].size();
                    Colorf color = mColorSample1[x][y];
                    glColor3f(color);
                    glVertex2f( tpos.x, tpos.y - scanPointL.y );
                }
                glEnd();
                
                if(0){

                    glLineWidth(2);
                    glColor3f(1, 0, 0);
                    glBegin( GL_LINES );
                    glVertex2f(scanPointL.x, -mExp.mFbo.getHeight()*10);
                    glVertex2f(scanPointL.x, 0);
                    glEnd();
                }
                
                glColor3f(1, 0, 0);
                glPointSize(5);
                glBegin( GL_POINTS );
                glVertex2f(scanPointL.x, 0);
                glEnd();
            }gl::popMatrices();
        }
        
        {
            float scanSpeed = 48;
            int head = frameR * scanSpeed;
            int next_head = (frameR+1) * scanSpeed;
            if( next_head >= allpointsR.size() ){
                quit();
            }
            
            Vec2f newScanPoint = allpointsR[next_head];
            Vec2f dir = newScanPoint - scanPointR;
            float dist = dir.length();
            if( scanSpeed/4 <= dist && dist< scanSpeed ){
                scanPointR = newScanPoint;
            }else if( scanSpeed <= dir.length()  ){
                scanPointR += dir.normalized()*scanSpeed;
            }else{
                scanPointR = newScanPoint;
                frameR++;
            }
            
            // RIGHT contour
            gl::pushMatrices();{
                glTranslatef( 600+3000, mExp.mFbo.getHeight()/2, 0);
                
                glPointSize(1);
                glBegin( GL_POINTS );
                for( int i=0; i<head; i++ ){
                    Vec2f & tpos = allpointsR[i];
                    int x = (int)abs(tpos.x) % mColorSample2.size();
                    int y = (int)abs(tpos.y) % mColorSample2[0].size();
                    Colorf color = mColorSample2[x][y];
                    glColor3f(color);
                    glVertex2f( tpos.x, tpos.y - scanPointR.y );
                }
                glEnd();

                if(0){

                    glLineWidth(2);
                    glColor3f(1, 0, 0);
                    glBegin( GL_LINES );
                    glVertex2f(scanPointR.x, -mExp.mFbo.getHeight()*10);
                    glVertex2f(scanPointR.x, 0);
                    glEnd();
                }
                
                glColor3f(1, 0, 0);
                glPointSize(5);
                glBegin( GL_POINTS );
                glVertex2f(scanPointR.x, 0);
                glEnd();
            }gl::popMatrices();
        }
        
        // LEFT & RIGHT RAIL
        if(0){
            glLineWidth(5);
            glColor3f(1, 0, 0);
            glBegin( GL_LINES );
            glVertex2f(120, 960);
            glVertex2f(1320, 960);
            
            glVertex2f(3000, 960);
            glVertex2f(4200, 960);
            glEnd();
        }
        
        
    } mExp.end();
    
    gl::color( Colorf::white() );
    mExp.draw();

    //mt::drawScreenGuide();
}

void cApp::keyDown( KeyEvent event ) {
    char key = event.getChar();
    switch (key) {
        case 'S':
            mExp.startRender();
            break;
        case 'T':
            mExp.stopRender();
            break;
            
        }
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
