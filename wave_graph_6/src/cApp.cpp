//#define RENDER

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Perlin.h"
#include "cinder/Rand.h"

#include "cinder/audio/Context.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"

#include "cinder/audio/Node.h"
#include "cinder/audio/GenNode.h"
#include "cinder/audio/GainNode.h"

#include "FftWaveWriter.h"
#include "ConsoleColor.h"
#include "MonitorNodeNL.h"
#include "mtUtil.h"
#include "SoundWriter.h"
#include "VboSet.h"
#include "Exporter.h"
#include "Wave.h"
#include "TbbNpFinder1.h"
#include "TbbNpFinder2.h"

#include "AudioOutput.h"

#include "tbb/tbb.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace tbb;

class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    void keyDown( KeyEvent e);
    void makePulse();
    void makeLine();
    void makeLine2();
    void makeLine3();
 
    bool debug = false;
    
    const unsigned int wW =  1080*4;
    const unsigned int wH =  1920;
    
    unsigned int frame = 0;
    unsigned int dispSample = 1920*2;
    unsigned int updateSample = 48000 / 25;
    unsigned int audioPos = 0;
    
    Vec2f L3 = { 200,  960 };   // left most point
    Vec2f R3 = { 4120, 960 };   // right most point
    float waveLength = R3.x - L3.x;
    
    Exporter    mExp;
    Perlin      mPln;
    vector<Wave> wave;
    vector<VboSet> vboWave;
    
    float xscale;
    float yscale;
    
    VboSet pulse;
    VboSet line;
    VboSet line2;
    VboSet line3;
    
    
    AudioOutputRef	mAudioOutput;
    
};


void cApp::setup(){
    
    setWindowSize( wW/2, wH/2 );
    setWindowPos( 0, 0 );
    setFrameRate(25);
    
    mExp.setup( wW, wH, 12000, GL_RGB, mt::getRenderPath(), 0);
    mPln.setOctaves(4);
    mPln.setSeed(4444);
    vector<fs::path> filePath;
    filePath.push_back( mt::getAssetPath()/"snd"/"timeline"/"n5_2mix.aif"  );
    filePath.push_back( mt::getAssetPath()/"snd"/"timeline"/"n5_6ch"/"n5_base.aif"  );
    filePath.push_back( mt::getAssetPath()/"snd"/"timeline"/"n5_6ch"/"n5_event.aif" );
    filePath.push_back( mt::getAssetPath()/"snd"/"timeline"/"n5_6ch"/"n5_sine.aif"  );

    wave.assign( filePath.size(), Wave() );
    for( int i=0; i<filePath.size(); i++ ){
        wave[i].load( filePath[i] );
        for( int ch=0; ch<wave[i].nCh; ch++)
            vboWave.push_back( VboSet() );
    }
    
    yscale = (float)wH * 0.6;

    makePulse();
    
#ifndef RENDER
    auto ctx = audio::master();
    auto gain = ctx->makeNode( new audio::GainNode( 0.5f ) );
    mAudioOutput = ctx->makeNode( new AudioOutput( audio::Node::Format().channels( 2 ) ) );
    mAudioOutput >> gain >> ctx->getOutput();
    mAudioOutput->setBuffer( wave[0].buf.get() );
    ctx->enable();
#endif
    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){
    
#ifndef RENDER
    mAudioOutput->setPosition( audioPos );
#endif
    
    for( auto & w : vboWave ){
        w.resetPos();
        w.resetCol();
        w.resetVbo();
    }
    
    bool remake = false;
    int vboIndex = 0;
    float pastd = 0;
    for ( int w=0; w<wave.size(); w++ ) {
        for ( int c=0; c<wave[w].nCh; c++ ) {

            float pastx = 0;
            xscale = (float)waveLength / (dispSample-1);

            const float * ch = wave[w].buf->getChannel( c );
            
            for ( int s=0; s<dispSample; s++) {
                float d = ch[audioPos + s];
                float red   = pulse.getCol()[s].r;
                float green = pulse.getCol()[s].g;
                float blue  = pulse.getCol()[s].b;
                float a  = pulse.getCol()[s].a;

                red = red*10.0f;

                // lowpass
                if( green > 0.5){
                    d = d*0.99999 + pastd*0.00001;
                    red = 1;
                }
                // lowres
                if( blue > 0.6){
                    d = d * 0.99999 + pastd * 0.00001;
                    d = (int)(d/0.01) * 0.01;
                    red = 1;
                    s += 4;
                }

                float minus = (d<0)?-1.0f:1.0f;
                
                if( red*green*blue>2.9){
                    d = (1.0f-abs(d));
                    d *= (-minus);
                }
                
                pastd = d;

                float offset = pulse.getPos()[s].y;

                float gap = 384/2;
                if( blue < -0.66){
                    offset -= gap*(w*2+c);
                }else if( 0.66<blue){
                    offset += gap*(w*2+c);
                }
                
                float inout = 1;
                
                float finalx = s * xscale * a*1.5;
                pastx = finalx;
                
                if (finalx>waveLength) {
                    break;
                }

                float area = 360;// waveLength*0.2;
                if( finalx<area ){
                    inout = finalx/area;
                    inout = pow(inout, 3);
                    offset = 0;
                    finalx += xscale*3;
                    red = 1;
                }else if( (waveLength-area)<finalx ){
                    inout = (waveLength-finalx)/area ;
                    inout = pow(inout, 3);
                    offset = 0;
                    finalx += xscale*3;
                    red = 1;
                }
                float finaly = d*yscale*inout*red + offset*inout;
                
                {
                    float lim = 384*2;
                    if( finaly < -lim){
                        finaly = -lim;
                        remake = true;
                    }else if( finaly>lim){
                        finaly = lim;
                        remake = true;
                    }
                }
                
                if( abs(d) > 0.0075 ){
                    vboWave[vboIndex].addPos( Vec3f( finalx, finaly, 0) );
                    vboWave[vboIndex].addCol( ColorAf(1,1,1,0.3) );
                }
            }
            
            vboWave[vboIndex].init( GL_POINTS );
            vboIndex++;
        }
    }
    
    //makeLine();
    makeLine2();
    makeLine3();
    
    if( remake )
        makePulse();
}

void cApp::makePulse(){
    
    pulse.resetVbo();
    
    xscale = (float)waveLength / (dispSample-1);
    float cnt = 0;
    float y = 0;
    float red = 0;
    float green = 0;
    float blue = 0;
    float alpha = 0;
    
    bool minus = true;
    
    for (int i=0; i<dispSample; i++) {

        int cntMax = dispSample*randFloat(0.002, 0.2);

        if( cnt >cntMax ){
            if( mPln.noise( 0.2+frame*0.01, i*0.2 )>0.15 ){
                float noise = abs( mPln.noise( frame*0.001, i*0.05 ) );
                noise *= minus?-1.0f:1.0f;
                if( abs(y) >= 2.0 ){
                    y = 2.0f*(y<0?-1.0f:1.0f);
                }
                
                // y = y*0.9 + noise*0.1;
                y += noise;
                minus = !minus;
                cnt = 0;
                
                red = randFloat();
                green = randFloat();
                blue = randFloat();
                alpha = randInt(1, 13); // 1~10
                alpha = pow(2, alpha);
                alpha *= 0.02f;   // 0.1 ~ 0.7
            }
        }else{
            cnt++;
        }
        
        y = (int)(y/0.1) * 0.1;
        
        pulse.addPos( Vec3f(i*xscale, y*wH*0.45, 0) );
        pulse.addCol( ColorAf(red,green,blue,1));
    }
    pulse.init(GL_POINTS);
}

void cApp::makeLine(){
    
    line.resetVbo();
    concurrent_vector<Vec3f> lpos;
    concurrent_vector<ColorAf> lcol;
    
    parallel_for( blocked_range<size_t>(0, 50000),
                 [&](blocked_range<size_t> & r){
        for( int i=r.begin(); i!=r.end(); ++i){
        
            int vid1 = randInt(0, vboWave.size() );
            int vid2 = randInt(0, vboWave.size() );
            
            VboSet & vbo1 = vboWave[vid1];
            VboSet & vbo2 = vboWave[vid2];
            
            const vector<Vec3f> & pos1 = vbo1.getPos();
            const vector<Vec3f> & pos2 = vbo2.getPos();
            
            int pid1 = randInt(0, pos1.size() );
            int pid2 = randInt(0, pos2.size() );
            
            const Vec3f & p1 = pos1[pid1];
            const Vec3f & p2 = pos2[pid2];
            
            Vec3f dir = p1 - p2;
            float dist = dir.length();
            Vec3f dirn = dir.normalized();
            
            if( 30<dist && dist<40.0){
                lpos.push_back( p1 );
                lpos.push_back( p2 );
                lcol.push_back( ColorAf(1,1,1,0.1) );
                lcol.push_back( ColorAf(1,1,1,0.1) );
            }
        }

    });
    
    vector<Vec3f> lpost;
    lpost.insert( lpost.begin(), lpos.begin(), lpos.end());

    vector<ColorAf> lcolt;
    lcolt.insert( lcolt.begin(), lcol.begin(), lcol.end() );
    
    line.addPos( lpost );
    line.addCol( lcolt );
    line.init( GL_LINES );
}

void cApp::makeLine2(){
    
    line2.resetVbo();

    int num_line = 20;
    int num_dupl = 4;
    int vertex_per_point = num_line * num_dupl * 2;
    
    vector<Vec3f> pos;
    vector<ColorAf> col;
    
    for( int i=0; i<vboWave.size()-1; i++){
        const vector<Vec3f> & vpos = vboWave[i].getPos();
        const vector<ColorAf> & vcol = vboWave[i].getCol();
        pos.insert(pos.end(), vpos.begin(), vpos.end() );
        col.insert(col.end(), vcol.begin(), vcol.end() );
    }
    
    vector<Vec3f> out;
    out.assign( pos.size()*vertex_per_point, Vec3f(-99999, -99999, 0) );
    
    vector<ColorAf> outc;
    outc.assign( pos.size()*vertex_per_point, ColorAf(0,0,0,0) );
    
    TbbNpFinder1 npf;
    npf.findNearestPoints( &pos[0], &out[0], &col[0], &outc[0], pos.size(), num_line, num_dupl );
    line2.addPos( out );
    line2.addCol( outc );
    
    line2.init( GL_LINES );
}

void cApp::makeLine3(){
    
    line3.resetVbo();
    
    int num_line = 4;
    int num_dupl = 3;
    int vertex_per_point = num_line * num_dupl * 2;
    
    vector<Vec3f> pos;
    vector<ColorAf> col;
    
    for( int i=0; i<vboWave.size()-1; i++){
        const vector<Vec3f> & vpos = vboWave[i].getPos();
        const vector<ColorAf> & vcol = vboWave[i].getCol();
        pos.insert(pos.end(), vpos.begin(), vpos.end() );
        col.insert(col.end(), vcol.begin(), vcol.end() );
    }
    
    vector<Vec3f> out;
    out.assign( pos.size()*vertex_per_point, Vec3f(-99999, -99999, 0) );
    
    vector<ColorAf> outc;
    outc.assign( pos.size()*vertex_per_point, ColorAf(0,0,0,0) );
    
    TbbNpFinder2 npf;
    npf.findNearestPoints( &pos[0], &out[0], &col[0], &outc[0], pos.size(), num_line, num_dupl );
    line3.addPos( out );
    line3.addCol( outc );
    
    line3.init( GL_LINES );
}
void cApp::draw(){
    
    gl::enableAlphaBlending();
    glPointSize(2);
    glLineWidth(1);

    mExp.beginOrtho(); {
        gl::pushMatrices(); {
            gl::clear( Color(0,0,0) );
            
            glTranslatef( L3.x, wH/2, 0);
            
            if( debug ) pulse.draw();
            
            line.draw();
            line2.draw();
            line3.draw();
        
            for( int i=0; i<vboWave.size(); i++ ){
                vboWave[i].draw();
            }
        } gl::popMatrices();

        gl::pushMatrices();{
          
            gl::color(1, 0, 0);
            glPointSize(5);
            glBegin(GL_POINTS);
            glVertex2f(L3.x, L3.y);
            glVertex2f(R3.x, R3.y);
            glEnd();
            
        } gl::popMatrices();

    } mExp.end();

    mExp.draw();
    
    gl::pushMatrices();
    stringstream ss;
    Vec2f tp(10,20);
    gl::color(1, 1, 1);

    gl::drawString( "audio  pos    : " + to_string(audioPos), tp);  tp.y+=30;
    gl::drawString( "visual frame  : " + to_string(frame), tp);     tp.y+=30;
    gl::drawString( "dispSample    : " + to_string(dispSample), tp);  tp.y+=30;
    gl::drawString( "updateSample  : " + to_string(updateSample), tp);  tp.y+=30;
    gl::drawString( "y scale       : " + to_string(yscale), tp);  tp.y+=30;
    gl::popMatrices();
    
    frame++;
    audioPos = frame*updateSample;
}

void cApp::keyDown(KeyEvent e){

#ifndef RENDER
    bool reset = false;
    switch( e.getCode() ){
        case e.KEY_RIGHT:   dispSample *= 2; reset=true; break;
        case e.KEY_LEFT:    dispSample /= 2; reset=true; break;
        case e.KEY_UP:      yscale *= 2;     break;
        case e.KEY_DOWN:    yscale /= 2;     break;
    }
    
    switch (e.getChar()) {
        case 'd': debug = !debug; break;
        case 'p': makePulse(); break;
        case 'S': mExp.startRender(); break;
        case 'f': frame += 25*30; break;
    }

    if( reset) makePulse();
    
#endif
    
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
