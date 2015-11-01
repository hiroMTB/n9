#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "cinder/audio/Context.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"

#include "mtUtil.h"
#include "AudioDrawUtils.h"
#include "SoundWriter.h"
#include "ConsoleColor.h"
#include "MonitorNodeNL.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();

    void keyDown( KeyEvent event );
    void mouseMove( MouseEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void resize();
 
    const int mFpb = 512;   // frames per block(audio buffer)
    int fftSize = 8;
    float smoothingFactor = 0.5;
    audio::dsp::WindowType windowType = audio::dsp::WindowType::BLACKMAN; //HAMMING; //BLACKMAN HAMMING, HANN, RECT

    vector<float> mSpc;
    vector< vector<float> > fftWaves;
    audio::BufferPlayerNodeRef mPlayer;
    audio::BufferRef buf;
    SpectrumPlot mSpectrumPlot;
    audio::MonitorSpectralNodeNLRef mMonitor;
    
    float * ch0;
    float * ch1;

};

void cApp::setup(){
    
    setFrameRate(10);
    setWindowSize( 720, 560 );
    setWindowPos( 0, 0 );
    gl::enableVerticalSync();
    
    {
        // Audio Setup
        auto ctx = audio::Context::master();
        audio::DeviceRef device = audio::Device::getDefaultOutput();
        audio::Device::Format format;
        format.sampleRate( 192000 );
        format.framesPerBlock( mFpb );
        device->updateFormat( format );
        
        cout << "--- Audio Setting --- \n";
        cout << "device name      : " << device->getName() << "\n";
        cout << "Sample Rate      : " << ctx->getSampleRate() << "\n";
        cout << "frames per Block : " << ctx->getFramesPerBlock() << "\n" << endl;
        
        audio::SourceFileRef sourceFile = audio::load( loadAsset( "snd/test/3s1e_192k.wav" ), ctx->getSampleRate() );
        buf = sourceFile->loadBuffer();

        cout << "--- Load audio file --- \n";
        cout << "sample num : " << buf->getNumFrames() << "\n";
        cout << "duration   : " << (float)buf->getNumFrames()/sourceFile->getSampleRate() << " sec\n"<< endl;
        
        ch0 = buf->getChannel(0);
        ch1 = buf->getChannel(1);

        mPlayer = ctx->makeNode( new audio::BufferPlayerNode(buf) );
        auto monitorFormat = audio::MonitorSpectralNodeNL::Format().fftSize( fftSize ).windowSize( fftSize/2 ).windowType( windowType);
        mMonitor = ctx->makeNode( new audio::MonitorSpectralNodeNL( monitorFormat ) );
        mMonitor->setSmoothingFactor( smoothingFactor );
        
        mPlayer >> ctx->getOutput();
        
        //
        //      need to conenct to initialize SpectramNodeNL
        //
        mPlayer >> mMonitor;
        //mPlayer->start();
        //mPlayer->seekToTime( mPlayer->getNumSeconds() );
        
        ctx->enable();
        
        fftWaves.assign( fftSize/2, vector<float>() );
    }
}

void cApp::update(){
    
    int audioPos = 0;
    int loop = 0;
    
    while( (audioPos+fftSize) < buf->getNumFrames() ){

        audioPos = loop * fftSize;
        
        float * fftch0 = new float[fftSize];
        float * fftch1 = new float[fftSize];
        
        memcpy( fftch0, ch0+audioPos, sizeof(float)*fftSize);
        memcpy( fftch1, ch1+audioPos, sizeof(float)*fftSize);
        
        audio::Buffer fftBlock(fftSize, 2);
        fftBlock.copyChannel( 0, fftch0 );
        fftBlock.copyChannel( 1, fftch1 );
        
        // This line calculate FFT
        mSpc = mMonitor->getMagSpectrum( fftBlock );
        
        for( int i=0; i<mSpc.size(); i++){
            float m = mSpc[i];
            float mD = audio::linearToDecibel(m)/100.0f;
            //mD = ( mD-0.5 ) * 2.0;
            fftWaves[i].push_back( mD );
        }
        
        loop++;
    }
    
        
    //
    //      write audio file at end of frame
    //
    {
        cout << "Write audio file ;" << endl;
        fs::path dir = mt::getRenderPath();
        createDirectories( dir.string()+"/" );
        
        for( int i=0; i<fftWaves.size(); i++ ){
            string fileName = "fft_" + toString(i) + ".wav";
            string path = dir.string() + "/" + fileName;
            
            int nCh = 1;
            
            // fftSize= 8, 24kHz
            // fftSize=16, 12kHz
            // fftSize=32,  6kHz
            int samplingRate = 192000/fftSize;
            if( SoundWriter::writeWav32f(fftWaves[i], nCh, samplingRate, fftWaves[i].size()/nCh, path) ){
                ccout::b("wav write OK :" + path + ", " + toString(fftWaves[i].size()) + " frames");
            }else{
                ccout::r("wav write ERROR :" + path);
            }
        }
        
        quit();
        
    }
    
}

//
//      debug use
//
void cApp::draw(){

    gl::clear( Color(0,0,0) );

    glPushMatrix(); {
        glTranslatef( 50, 0, 0 );
        float scalex = 0.5;
        float scaley = 20;
        
        std::vector<ci::Vec2f>	verts;
        std::vector<ci::ColorA>	colors;
        
        glColor3f( 0, 1, 0 );
        glPointSize( 1 );
        glBegin( GL_POINTS );
        for( int i=0; i<fftWaves.size(); i++){
            for( int j=0; j<fftWaves[i].size(); j++ ){
                Vec2f v(j*scalex, -fftWaves[i][j]*scaley + (scaley*2.2)*i);
                glVertex2f( v );
            }
        }
        glEnd();
    } glPopMatrix();
}

void cApp::keyDown( KeyEvent event ){
}

void cApp::mouseDown( MouseEvent event ){
}

void cApp::mouseMove( MouseEvent event ){
}

void cApp::mouseDrag( MouseEvent event ){
}

void cApp::resize(){
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
