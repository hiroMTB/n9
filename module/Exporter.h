#pragma once

#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/Camera.h"
#include "ufUtil.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Exporter{
    
public:
    
    bool bRender;
    bool bSnap;
    int mFrame;
    int mExitFrame;
    gl::Fbo mFbo;
    fs::path mRenderPath;
    ImageTarget::Options mImgWOption;
    
    Exporter(){}
    
    void setup( int width, int height, int exitFrame, GLenum colorInternalFormat, fs::path path, int aaSample ){
        bRender = false;
        bSnap = false;
        mFrame = 1;
        mExitFrame = exitFrame;
        gl::Fbo::Format format;
        format.enableDepthBuffer( false );
        format.enableMipmapping( false );
        format.setTarget( GL_TEXTURE_RECTANGLE_ARB );
        format.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );
        format.setMinFilter( GL_NEAREST );
        format.setMagFilter( GL_NEAREST );
        format.setColorInternalFormat( colorInternalFormat );
        format.setSamples( aaSample );
        mFbo = gl::Fbo( width, height, format );
        //mFbo.getTexture(0).setFlipped(true);
        mRenderPath = path;
        
        mImgWOption.quality(1);
        
        stringstream ss;
        ss << "setup FBO Renderer " << width << " x " << height << "px\n";
        ss << "exitFrame " << exitFrame << ", AntiAlias " << aaSample << "\n";
        ss << "renderPath " << mRenderPath;
        cout << ss.str() << endl;
    }
    
    void begin(){
        gl::pushMatrices();
        //gl::SaveFramebufferBinding bindingSaver;
        gl::setViewport( mFbo.getBounds() );
        
        mFbo.bindFramebuffer();
        gl::setMatricesWindow( mFbo.getSize() );
    }

    void begin( const Camera & cam){
        gl::pushMatrices();
        //gl::SaveFramebufferBinding bindingSaver;
        gl::setViewport( mFbo.getBounds() );
        
        mFbo.bindFramebuffer();
        gl::setMatrices( cam );
    }

    void end(){
        mFbo.unbindFramebuffer();
        gl::popMatrices();
        
        if( bRender || bSnap ){
            string frame_name = "f_" + toString( mFrame ) + ".png";
            writeImage( mRenderPath/frame_name,  mFbo.getTexture());
            cout << "Render Image : " << mFrame << endl;
            
            if( mExitFrame <= mFrame ){
                if( bSnap ){
                    cout << "Finish Snapshot " << endl;
                }else{
                    cout << "Finish Rendering " << mFrame << " frames" << endl;
                    exit(1);
                }
            }
            
            bSnap = false;
            mFrame++;
        }
    }
    
    void startRender(){
        bRender = true;
        mFrame = 1;
    }
    
    void stopRender(){
        bRender = false;
        cout << "Stop Render : f_" << mFrame << endl;
    }
    
    void snapShot(){
        bSnap = true;
    }
    
    void draw(){

//        float win_w = getWindowWidth();
//        float win_h = getWindowHeight();
//        float win_aspect = win_w/win_h;
//        float fbo_aspect = mFbo.getAspectRatio();
//        Area area(0,0,0,0);
//        if( win_aspect >= fbo_aspect ){
//            float scale = mFbo.getHeight()/win_h;
//            area.set(0, 0, win_w * scale, win_h);
//        }else{
//            float scale = mFbo.getWidth() / win_w;
//            area.set(0, 0, win_w, win_h*scale);
//        }

        gl::pushMatrices();
        gl::setMatricesWindow( mFbo.getSize() );
        gl::setViewport(getWindowBounds() );
        gl::draw( mFbo.getTexture() );
        gl::popMatrices();
    }
};

