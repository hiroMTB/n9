#pragma once

#include "cinder/app/AppNative.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"

#include "ConsoleColor.h"
#include "MonitorNodeNL.h"
#include "mtUtil.h"
#include "SoundWriter.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class FftWaveWriter{
    
public:
    
    fs::path render_path;
    
    void write( audio::Context * ctx, fs::path src_path, int targetSampleRate=48000, float smoothingFactor=0.5, int fftSize=16, audio::dsp::WindowType windowType=audio::dsp::WindowType::BLACKMAN){
        
        
        // load source file with native sample rate
        DataSourceRef dsr = loadFile( src_path );
        audio::SourceFileRef sourceFile = audio::load( dsr );
        string src_name = src_path.filename().replace_extension().string();
        audio::BufferRef buf = sourceFile->loadBuffer();
        int nCh = sourceFile->getNumChannels();
        
        cout << "\n--- Load audio file --- \n";
        cout << "ch          : " << nCh << "\n";
        cout << "sample num  : " << buf->getNumFrames() << "\n";
        cout << "sample rate : " << sourceFile->getSampleRate() << "\n";
        cout << "duration    : " << (float)buf->getNumFrames()/sourceFile->getSampleRate() << " sec\n"<< endl;
        
        
        // make Monitor Node for FFT
        audio::MonitorSpectralNodeNLRef mMonitor;
        auto monitorFormat = audio::MonitorSpectralNodeNL::Format().fftSize( fftSize ).windowSize( fftSize/2 ).windowType( windowType);
        mMonitor = ctx->makeNode( new audio::MonitorSpectralNodeNL( monitorFormat ) );
        mMonitor->setSmoothingFactor( smoothingFactor );
        
        // make dummy player node
        audio::BufferPlayerNodeRef mPlayer = ctx->makeNode( new audio::BufferPlayerNode(buf) );
        mPlayer >> mMonitor;
        ctx->enable();

        // data container
        vector< vector<float> > fftWaves;
        fftWaves.assign( fftSize/2, vector<float>() );
        
        
        {
            // Fill data
            int audioPos = 0;
            int loop = 0;
            while( (audioPos+fftSize) < buf->getNumFrames() ){
                
                audioPos = loop * fftSize;
                audio::Buffer fftBlock(fftSize, nCh);
                
                for( int c=0; c<nCh; c++ ){
                    float * origch = buf->getChannel( c );
                    float fftch[fftSize];
                    memcpy( fftch, origch+audioPos, sizeof(float)*fftSize);
                    fftBlock.copyChannel( c, fftch );
                }
                
                // This line calculate FFT
                vector<float> mMag = mMonitor->getMagSpectrum( fftBlock );
                
                for( int i=0; i<mMag.size(); i++){
                    float m = mMag[i];
                    float mD = audio::linearToDecibel(m)/100.0f;
                    //mD = ( mD-0.5 ) * 2.0;
                    fftWaves[i].push_back( mD );
                }
                
                loop++;
            }
        }
        
        {
            // Write data
            
            if( render_path.empty() )
                render_path = mt::getRenderPath();
            
            cout << "Write audio file :" << endl;
            fs::path dir = render_path / src_name;
            createDirectories( dir.string()+"/" );
            
            for( int i=0; i<fftWaves.size(); i++ ){
                
                char c[255];
                sprintf(c, "%s_sm%0.1f_%02d-%02d.wav", src_name.c_str(), smoothingFactor, fftSize, i+1);
                string fileName = toString(c);
                string path = dir.string() + "/" + fileName;
                
                if( SoundWriter::writeWav32f(fftWaves[i], 1, targetSampleRate, fftWaves[i].size()/1, path) ){
                    ccout::b( "wav write OK :" + path + ", " + toString(fftWaves[i].size()) + " frames");
                }else{
                    ccout::r("wav write ERROR :" + path);
                }
                
                fftWaves[i].clear();
            }
        }
        
        buf.reset();
        mMonitor->disconnectAll();
        mPlayer->disconnectAll();
        ctx->disconnectAllNodes();
        sourceFile.reset();
    }
    
};

