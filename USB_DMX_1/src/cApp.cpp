#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "DMXPro.h"

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
    
    DMXProRef	mDmxController;
};

void cApp::setup(){
    setWindowSize( 1920, 1080 );
    setWindowPos( 0, 0 );
    
    // list all the available serial devices
    // DMX Pro is usually something like "tty.usbserial-EN.."
    DMXPro::listDevices();
    
    // create a device passing the device name or a partial device name
    // useful if you want to swap device without changing the name
    mDmxController = DMXPro::create( "tty.usbserial-EN" );
    
    if ( mDmxController->isConnected() )
        console() << "DMX device connected" << endl;
}

void cApp::update(){
    
    int dmxChannel	= 2;
    int dmxValue 	= 255;
    
    if ( mDmxController && mDmxController->isConnected() )
    {
        // send value 255 to channel 2
        mDmxController->setValue( dmxValue, dmxChannel );
    }
}

void cApp::draw(){

    gl::clear();
    
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
