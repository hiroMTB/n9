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
        samplingRate = sourceFile->getSampleRate();
        buf = sourceFile->loadBuffer();
        ch0 = buf->getChannel(0);
    }
    
    audio::BufferRef buf;
    float * ch0;
    int samplingRate;
    vector<Vec3f> pos;
    vector<ColorAf> color;
    
};
