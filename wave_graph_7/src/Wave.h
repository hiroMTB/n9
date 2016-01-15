#pragma once

#include "cinder/app/AppNative.h"
#include "cinder/Utilities.h"

#include "cinder/audio/Context.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Wave{
    
public:
 
    Wave(){};
    
    void load( fs::path path ){
        DataSourceRef ref = loadFile( path );
        audio::SourceFileRef sourceFile = audio::load( ref );
        samplingRate = sourceFile->getSampleRate();
        buf = sourceFile->loadBuffer();
        nCh = sourceFile->getNumChannels();
    }
    
    audio::BufferRef buf;
    int samplingRate;
    int nCh;
    
};
