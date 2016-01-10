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
    
    void create( fs::path path, int _update_num ){
        update_num = _update_num;
        DataSourceRef ref = loadFile( path );
        audio::SourceFileRef sourceFile = audio::load( ref );
        samplingRate = sourceFile->getSampleRate();
        buf = sourceFile->loadBuffer();
        numCh = sourceFile->getNumChannels();
        pos.assign(numCh, vector<Vec2f>() );
        col.assign(numCh, vector<ColorAf>() );
        for( auto &p : pos ) p.assign(update_num, Vec2f(0,0) );
        for( auto &c : col ) c.assign(update_num, ColorAf(0,0,0,1) );
    }
    
    void updatePos( int start ){
        
        for( int c=0; c<numCh; c++ ){
            const float * ch = buf->getChannel( c );
            
            for ( int i=0; i<update_num; i++) {
                long index = start + i;
                if( index < buf->getNumFrames() ){
                    float value = ch[index];
                    pos[c][i] = Vec2f(i, value);
                }else{
                    pos[c][i] = Vec2f(0,0);
                }
            }
        }
    }

    void clear(){        
        for( auto &p : pos ) p.clear();
        for( auto &c : col ) c.clear();
    }
    
    audio::BufferRef buf;
    int samplingRate;
    int numCh;
    int update_num;
    vector<vector<Vec2f>> pos;
    vector<vector<ColorAf>> col;
    
};
