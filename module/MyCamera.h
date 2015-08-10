#pragma once

#include "cinder/app/AppNative.h"
#include "cinder/params/Params.h"
#include "cinder/Camera.h"

using namespace ci;
using namespace ci::app;
using namespace std;


class MyCamera : public CameraPersp {
    
public:
    MyCamera();
    void setup();
    void drawParam();
    void updateCamera();
    
    //void loadParameters( string filename );
    //void saveParameters( string filename );
    
    params::InterfaceGlRef mParams;
};