//
//  StrangeAgent.h
//
//	Strange Attractor Node
//
//  spiroLive
//
//  Created by mtb on 4/30/14.
//

#pragma once

#include "cinder/app/AppNative.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class StrangeAgent{

public:
    StrangeAgent();
    
    void init( float a, float b, float c, float d, float e, float f );
    void update( float * ret=NULL );
    void draw();
    void setRandom();
    void toggleMute(){ mute = !mute; }

    static const int dotNumMax = 1024*10;
    bool mute;
    int head;
    float a,b,c,d,e,f;
    float freq;
    float phase, phaseAdder;
    
    vector<Vec3f> points;
    vector<ColorAf> colors;
};

