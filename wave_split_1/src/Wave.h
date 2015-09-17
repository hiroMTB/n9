#pragma once

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Vbo.h"
#include "cinder/Utilities.h"

#include "cinder/audio/Context.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Wave{
    
    
public:
 
    
    Wave(){};
    
    void create( string path, bool aUseVbo=true ){

        mUseVbo = aUseVbo;
        
        auto ctx = audio::Context::master();

        audio::SourceFileRef sourceFile = audio::load( loadAsset( path ), ctx->getSampleRate() );
        buf = sourceFile->loadBuffer();
        player = ctx->makeNode( new audio::BufferPlayerNode(buf) );
        monitor = ctx->makeNode( new audio::MonitorNode() );
        
        player >> ctx->getOutput();
        player >> monitor;
        player->start();
        
        int fpb = ctx->getFramesPerBlock();

        if( mUseVbo ){
            gl::VboMesh::Layout layout;
            layout.setDynamicColorsRGBA();
            layout.setDynamicPositions();
            layout.setStaticIndices();
            vboL = gl::VboMesh( fpb, 0, layout, GL_POINTS );
            vboR = gl::VboMesh( fpb, 0, layout, GL_POINTS );
        }
        
        posL.assign( fpb, Vec3f(0,0,0) );
        posR.assign( fpb, Vec3f(0,0,0) );
        
        colorL.assign( fpb, ColorAf(1,1,1,1) );
        colorR.assign( fpb, ColorAf(1,1,1,1) );

    }
    
    void update( int startFrame, int nFrames, Vec2f scale ){
        
        const float * ch0 = buf->getChannel( 0 );
        const float * ch1 = buf->getChannel( 1 );
        
        gl::VboMesh::VertexIter itrL(vboL);
        gl::VboMesh::VertexIter itrR(vboR);
        
        for ( int i=0; i<nFrames; i++) {
            float l = ch0[ startFrame + i];
            float r = ch1[ startFrame + i];
            
            posL[i] = Vec3f(i*scale.x, l*scale.y, 0);
            posR[i] = Vec3f(i*scale.x, r*scale.y, 0);
        
            if( mUseVbo ){
                itrL.setPosition( posL[i] );
                itrR.setPosition( posR[i] );
                
                itrL.setColorRGBA( colorL[i] );
                itrR.setColorRGBA( colorR[i] );
                
                ++itrL;
                ++itrR;
            }
        }
    }
    
    void drawL(){
        gl::draw( vboL );
    }

    void drawR(){
        gl::draw( vboR );
    }
    

    bool mUseVbo;
    
    audio::BufferRef buf;
    audio::MonitorNodeRef monitor;
    audio::BufferPlayerNodeRef player;
    
    vector<Vec3f> posL;
    vector<Vec3f> posR;
    
    vector<ColorAf> colorL;
    vector<ColorAf> colorR;

    gl::VboMesh vboL;
    gl::VboMesh vboR;
};
