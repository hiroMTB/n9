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
    
    void create( fs::path path ){
        DataSourceRef ref = loadFile( path );
        audio::SourceFileRef sourceFile = audio::load( ref );
        samplingRate = sourceFile->getSampleRate();
        buf = sourceFile->loadBuffer();
    }
    
    void update( int start, int num ){
        
        const float * ch0 = buf->getChannel( 0 );
        
        for ( int i=0; i<num; i++) {
            long index = start + i;
            if( index < buf->getNumFrames() ){
                float l = ch0[index];
                pos[i] = Vec3f(i, l, 0);
            }else{
                pos[i] = Vec3f(0,0,0);
            }
        }
    }

    audio::BufferRef buf;
    int samplingRate;
    vector<Vec3f> pos;
    vector<ColorAf> color;
    
};
