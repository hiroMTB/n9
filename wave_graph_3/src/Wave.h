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
        audio::SourceFileRef sourceFile = audio::load( loadAsset( path ) );
        samplingRate = sourceFile->getSampleRate();
        buf = sourceFile->loadBuffer();
    }
    
    void update( int start, int num ){

        chL.clear();
        const float * ch0 = buf->getChannel( 0 );
        
        for ( int i=0; i<num; i++) {
            long index = start + i;
            if( index < buf->getNumFrames() ){
                chL.push_back( ch0[index] );
            }else{
                chL.push_back( 0 );
            }
        }
    }
    
    audio::BufferRef buf;
    int samplingRate;
    vector<float> chL;
    vector<ColorAf> color;
    
};
