#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "cinder/audio/Context.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"

#include "FftWaveWriter.h"
#include "ConsoleColor.h"
#include "MonitorNodeNL.h"
#include "ufUtil.h"
#include "SoundWriter.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
  public:
	void setup();
    void draw();
    const int mFpb = 512;   // frames per block(audio buffer)
    
};

void cApp::setup(){
    
    setWindowSize( 720, 560 );
    setWindowPos( 0, 0 );
 
    // Audio Setup
    auto ctx = audio::Context::master();
    audio::DeviceRef device = audio::Device::getDefaultOutput();
    
    vector<fs::path> pList;
    
    fs::path dir = loadAsset("snd/samples/192k/")->getFilePath();
    
    fs::recursive_directory_iterator it(dir), eof;
    while( it!= eof){
        if( !fs::is_directory(it.status() ) ){
    
            if( it->path().filename().string().at(0) != '.' ){
                cout << "add to process list : " << pList.size() << " " << it->path().filename() << endl;
                pList.push_back( *it );
            }
        }
        
        ++it;
    }

    FftWaveWriter fww;
    for( auto path : pList ){
        fww.write( ctx, path, 48000, 0.5f, 16 );
        
//        for( int i=0; i<3; i++ ){
            //int fftSize = pow(2, i+3);
            //fww.write( ctx, path, 48000, 0.0f, fftSize );   // 8
            //fww.write( ctx, path, 48000, 0.5f, fftSize );   // 16
            //fww.write( ctx, path, 48000, 0.9f, fftSize );   // 32
//        }
    }

    ccout::b("\n\n\n finish audio file rendering \n\n");
}

void cApp::draw(){
    gl::clear( Color(0,0,0) );
    gl::color( 1, 1, 1 );
    gl::drawString("finish audio file rendering", Vec2f( 50, 100 ) );
    
    quit();
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
