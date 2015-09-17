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
    
    void create( string path ){

        auto ctx = audio::Context::master();

        audio::SourceFileRef sourceFile = audio::load( loadAsset( path ), ctx->getSampleRate() );
        buf = sourceFile->loadBuffer();
        player = ctx->makeNode( new audio::BufferPlayerNode(buf) );
        monitor = ctx->makeNode( new audio::MonitorNode() );
        
        player >> ctx->getOutput();
        player >> monitor;
        player->start();
        
        
        gl::VboMesh::Layout layout;
        layout.setDynamicColorsRGBA();
        layout.setDynamicPositions();
        layout.setStaticIndices();
        
        
        int fpb = ctx->getFramesPerBlock();
        vboL = gl::VboMesh( fpb, 0, layout, GL_POINTS );
        vboR = gl::VboMesh( fpb, 0, layout, GL_POINTS );
        
        posL.assign( fpb, Vec3f(0,0,0) );
        posR.assign( fpb, Vec3f(0,0,0) );
        
        colorL.assign( fpb, ColorAf(1,1,1,1) );
        colorR.assign( fpb, ColorAf(1,1,1,1) );

    }
    
    void update( Vec2f scale ){
        
        const audio::Buffer &cBuffer = monitor->getBuffer();
        
        int nFrames = cBuffer.getNumFrames();
        const float * ch0 = cBuffer.getChannel( 0 );
        const float * ch1 = cBuffer.getChannel( 1 );
        
        gl::VboMesh::VertexIter itrL(vboL);
        gl::VboMesh::VertexIter itrR(vboR);
        
        for ( int i=0; i<nFrames; i++) {
            float l = ch0[i];
            float r = ch1[i];
            
            posL[i] = Vec3f(i*scale.x, l*scale.y, 0);
            posR[i] = Vec3f(i*scale.x, r*scale.y, 0);
            
            itrL.setPosition( posL[i] );
            itrR.setPosition( posR[i] );
            
            itrL.setColorRGBA( colorL[i] );
            itrR.setColorRGBA( colorR[i] );

            ++itrL;
            ++itrR;
        }
    }
    
    void drawL(){
        gl::draw( vboL );
    }

    void drawR(){
        gl::draw( vboR );
    }
    
    
    audio::BufferRef buf;
    audio::MonitorNodeRef monitor;
    audio::BufferPlayerNodeRef player;
    
    gl::VboMesh vboL;
    gl::VboMesh vboR;
    
    
    vector<Vec3f> posL;
    vector<Vec3f> posR;
    
    vector<ColorAf> colorL;
    vector<ColorAf> colorR;
    
};
