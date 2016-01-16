#define RENDER
//#defin AUDIO

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
#include "Axis.h"
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
    void drawAxis();
    void keyDown( KeyEvent e);
    void makePulse();
    void makeLine();
    void makeLine2();
    void makeLine3();
    void makePrep();
    void drawInfo();
    void loadCsv( fs::path path, vector<vector<float>> & array, bool print=false );
    void setupFromBlender();

    
    bool debug = false;
    
    const unsigned int wW =  1080*4;
    const unsigned int wH =  1920;
    
    unsigned int frame = 6845 + 50;
    unsigned int endFrame = 7096;
    
    unsigned int dispSample = 1920*4;
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
    float yscale = 7000; //2600;

    
    VboSet pulse;
    VboSet line;
    VboSet line2;
    VboSet line3;
    VboSet prep;
    
    AudioOutputRef	mAudioOutput;
    
    fs::path assetDir = mt::getAssetPath();
    vector<n5::Axis> axis;
    
    vector<Vec2i> leftpos{
        Vec2i(480,192), Vec2i(840,576), Vec2i(120,960), Vec2i(840,1344), Vec2i(480,1728),       // L
        Vec2i(2640,192), Vec2i(2280,576), Vec2i(3000,960), Vec2i(2280,1344), Vec2i(2640,1728)   // R
    };
    
    vector<Vec2i> pivot;

};

void cApp::setupFromBlender(){
    
    vector<vector<float>> csv;
    loadCsv( assetDir/"csv"/"n5_timeline_v9.1_allframe.csv", csv );
    
    // fill
    for( int i=0; i<10; i++){
        axis.push_back( n5::Axis() );
        axis[i].left = leftpos[i];
        axis[i].animPos.assign( 12000, Vec2f(0,0) );
        axis[i].animPower.assign( 12000, 0 );
    }
    
    for( int i=0; i<12000; i++){
        for( int j=0; j<10; j++){
            float xpos = csv[i][j*2];
            int power = csv[i][j*2+1];
            int y = axis[j].left.y;
            axis[j].animPos[i].x = xpos;
            axis[j].animPos[i].y = y;
            axis[j].animPower[i] = power;
        }
    }
}

void cApp::loadCsv( fs::path path, vector<vector<float>> & array, bool print ){
    
    ifstream file ( path.string() );
    if( !file.is_open() ){
        cout << "cant open file" << endl;
    }else{
        std::string line;
        while ( std::getline( file, line ) ){
            
            std::istringstream iss{ line };
            std::vector<float> tokens;
            std::string cell;
            while( std::getline(iss, cell, ',') ){
                cell.erase( std::remove(cell.begin(), cell.end(), '\r'), cell.end() );  // erase \r
                float cell_f = fromString<float>(cell);
                tokens.push_back( cell_f );
            }
            
            array.push_back( tokens );
        }
    }
    
    if( print ){
        for (auto & line : array){
            for (auto & val : line){
                printf( "%f ", val);
            }
            printf("\n");
        }
    }
}

void cApp::setup(){
    
    setWindowSize( wW/2, wH/2 );
    setWindowPos( 0, 0 );
    
    mExp.setup( wW, wH, 0, 12000-1, GL_RGB, mt::getRenderPath(), 0, "wg6_");
    mPln.setOctaves(4);
    mPln.setSeed( mt::getSeed() );
    
    setupFromBlender();
    
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
    
  
    makePulse();

    
    
#ifndef RENDER
#ifdef AUDIO
    
    auto ctx = audio::master();
    auto gain = ctx->makeNode( new audio::GainNode( 0.5f ) );
    mAudioOutput = ctx->makeNode( new AudioOutput( audio::Node::Format().channels( 2 ) ) );
    mAudioOutput >> gain >> ctx->getOutput();
    mAudioOutput->setBuffer( wave[0].buf.get() );
    ctx->enable();
#endif
#endif
    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){
    
    
    if(frame>=endFrame) quit();
    
#ifndef RENDER
#ifdef AUDIO
    mAudioOutput->setPosition( audioPos );
#endif
#endif
    
    
    for( auto & w : vboWave ){
        w.resetPos();
        w.resetCol();
        w.resetVbo();
    }
    
    pivot.clear();
    for( int i=0; i<axis.size(); i++){
        axis[i].update( frame );
        if( axis[i].power > 200 ){
            pivot.push_back( axis[i].pos );
        }
    }
    
    if( pivot.size() == 0){
        pulse.resetVbo();
        prep.resetVbo();
        line.resetVbo();
        line2.resetVbo();
        line3.resetVbo();
        return;
    }

    bool remake = false;

    int vboIndex = 0;
    float pastd = 0;
    for ( int w=0; w<wave.size(); w++ ) {
        for ( int c=0; c<wave[w].nCh; c++ ) {

            float pastx = 0;
            xscale = (float)waveLength / (dispSample-1);

            const float * ch = wave[w].buf->getChannel( c );
            
            //for ( int s=0; s<dispSample; s++) {
            int s = 0;
            while( pastx < waveLength ){
                
                if( (audioPos+s) > 8*60*25*48000 )
                    break;
                
                float d = ch[audioPos + s];
                float red   = pulse.getCol()[s].r;
                float green = pulse.getCol()[s].g;
                float blue  = pulse.getCol()[s].b;
                float a  = pulse.getCol()[s].a;

                red = red*1.4f;

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
                
                if( red*green*blue>2.9){
                    d = 1;
                }
                
                pastd = d;

                float offset = pulse.getPos()[s].y * 2.2;

                float gap = 384/8/2;
                if( 0.5<a && a< 0.9){
                        switch( vboIndex ){
                            case 0 : d = 1; break;
                            case 1 : offset += gap*1; d=abs(d); break;
                            case 2 : offset += gap*2; /*d=1.0-abs(d);*/     break;
                            case 3 : offset += gap*3; d=d*d*d;    break;
                            case 4 : offset -= gap*1; break;
                            case 5 : offset -= gap*2; break;
                            case 6 : offset -= gap*3; break;
                            case 7 : d = -1; break;
                        }
                }
                
                float inout = 1;
                float aa = lmap(a, 0.0f, 1.0f, 0.5f, 12.0f);
                float finalx = pastx + xscale*(aa);
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
                
//                if( s==0 ){
//                    finaly = pivot[0].y;
//                }
                
//                {
//                    float lim = 384*2;
//                    if( finaly < -lim){
//                        finaly = -lim;
//                        remake = true;
//                    }else if( finaly>lim){
//                        finaly = lim;
//                        remake = true;
//                    }
//                }
                
                if( abs(d) > 0.01 ){
                    vboWave[vboIndex].addPos( Vec3f( finalx, finaly, 0) );
                    vboWave[vboIndex].addCol( ColorAf(1,1,1,0.3) );
                }
                s++;

            }
            
            vboWave[vboIndex].init( GL_POINTS );
            vboIndex++;
        }
    }
    
    //makeLine();
    makeLine2();
    makeLine3();
    
    if( remake ){
        makePulse();
    }
    
    makePrep();
}

void cApp::makePulse(){
    
    pulse.resetVbo();
    
    xscale = (float)waveLength / (dispSample-1);
    float cnt = 10;
    float y = 0;
    float red = 0;
    float green = 0;
    float blue = 0;
    float alpha = 0;
    
    bool minus = true;
    
    for (int i=0; i<dispSample; i++) {

        int cntMax = dispSample*randFloat(0.01, 0.4);

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
                alpha = randInt(1, 8);
                alpha = pow(2, alpha);
                alpha /= pow(2,7);
            }
        }else{
            int range = dispSample*0.1;
            if( i<range || i<dispSample-range){
                cnt+=5;
            }else{
                cnt++;
            }
        }
        
        y = (int)(y/0.1) * 0.1;
        
        float xpos = i*xscale;
        //if( vboWave[0].getPos().size() > i ){
        //    xpos = vboWave[0].getPos()[i].x;
        //}
        pulse.addPos( Vec3f(xpos, y*wH*0.45, 0) );
        pulse.addCol( ColorAf(red,green,blue,alpha));
    }
    pulse.init(GL_POINTS);
}

void cApp::makePrep(){
    
    prep.resetVbo();
    
    for( int i=0; i<vboWave.size(); i+=2){
        for( int j=0; j<vboWave[i].getPos().size(); j++){
            const Vec3f & p1 = pulse.getPos()[j];
            const Vec3f & p2 = vboWave[i].getPos()[j];
    
            float x = p2.x;
            float y = p1.y;
            float area = 360;
            float inout = 1;
            if( x<area ){
                inout = x/area;
                inout = pow(inout, 3) * 0.1;
                y *= inout;
            }else if( (waveLength-area)<x ){
                inout = (waveLength-x)/area ;
                inout = pow(inout, 3) * 0.1;
                y *= inout;
            }

            prep.addPos( Vec3f( x, y, 0) );
            prep.addPos( p2 );
            prep.addCol( ColorAf(1,1,1,0.15) );
            prep.addCol( ColorAf(1,1,1,0.15) );
        }
    }
    
    prep.init( GL_LINES );
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
    
    TbbNpFinder2 npf;
    npf.findNearestPoints( &pos[0], &out[0], &col[0], &outc[0], pos.size(), num_line, num_dupl );
    line3.addPos( out );
    line3.addCol( outc );
    
    line3.init( GL_LINES );
}

bool ortho = true;

void cApp::draw(){
    
    gl::enableAlphaBlending();
    glPointSize(1);
    glLineWidth(1);

    ortho ? mExp.beginOrtho() : mExp.beginPersp();
    {
        gl::pushMatrices(); {
            gl::clear( Color(0,0,0) );
            
            glTranslatef( L3.x, wH/2, 0);
            
            if( debug ) pulse.draw();

            prep.draw();
            line.draw();
            line2.draw();
            line3.draw();
        
            for( int i=0; i<vboWave.size(); i++ ){
                vboWave[i].draw();

                const vector<Vec3f> & v = vboWave[i].getPos();
                for( int k=1; k<=3; k++ ){
                    for( int j=0; j<v.size(); j++ ){
                        Vec3f vv = v[j] + mPln.dfBm(frame*0.1+k*0.33, 0.1+i*0.001, 0.1*k+j*0.1)*0.2;
                        vboWave[i].writePos(j, vv);
                    }
                    vboWave[i].draw();
                }
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
    {
        gl::scale(0.5, 0.5);
        drawAxis();
    }
    gl::popMatrices();


    drawInfo();

    frame++;
    audioPos = frame*updateSample;
}

void cApp::drawInfo(){
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
}

void cApp::keyDown(KeyEvent e){

#ifndef RENDER
    bool reset = false;
    switch( e.getCode() ){
        case e.KEY_RIGHT:   frame++; break; //dispSample *= 2; reset=true; break;
        case e.KEY_LEFT:    frame--; break; //dispSample /= 2; reset=true; break;
        case e.KEY_UP:      yscale *= 2;     break;
        case e.KEY_DOWN:    yscale /= 2;     break;
    }
    
    switch (e.getChar()) {
        case 'd': debug = !debug; break;
        case 'p': makePulse(); break;
        case 'S': mExp.startRenderFrom( frame ); break;
        case 'f': frame += 25*10; break;
        case 'o': ortho = !ortho; break;
    }

    if( reset) makePulse();
    
#endif
    
}

void cApp::drawAxis(){
    for( int i=0; i<axis.size(); i++){
        
        const n5::Axis & ax = axis[i];
        
        glPointSize(5);
        glLineWidth(2);
        glColor3f( 0.1,0.1,0.1 );
        
        // draw rail
        glBegin( GL_LINES );
        glVertex3f( ax.left.x, ax.left.y, 0 );
        glVertex3f( ax.left.x+ax.length, ax.left.y, 0);
        glEnd();
        
        // draw point
        bool on = (ax.power>200);
        on ? glColor3f( 1,0,0 ) : glColor3f( 0.3,0.3,0.3 );
        if( on ) gl::drawStrokedCircle(ax.pos, 30);
        
        glBegin( GL_POINTS );
        glVertex2f( ax.pos );
        glEnd();
    }

}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
