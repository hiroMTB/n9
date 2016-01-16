#define RENDER

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"

#include "cinder/audio/Context.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"
#include "cinder/ImageIo.h"
#include "cinder/Perlin.h"
#include "cinder/rand.h"
#include "cinder/BSpline.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"

#include "boost/polygon/voronoi.hpp"
#include "boost/polygon/segment_data.hpp"

#include "mtUtil.h"
#include "mtSvg.h"
#include "Wave.h"
#include "Exporter.h"

using namespace ci;
using namespace ci::app;
using namespace std;

using boost::polygon::voronoi_builder;
using boost::polygon::voronoi_diagram;
typedef boost::polygon::point_data<float> vPoint;
typedef boost::polygon::segment_data<float> vSegment;
typedef voronoi_diagram<double>::cell_type cell_type;
typedef voronoi_diagram<double>::edge_type edge_type;
typedef voronoi_diagram<double>::vertex_type vertex_type;


class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    void draw_voronoi();
    
    const int win_w = 4320;
    const int win_h = 1920;

    vector<Perlin> mPlns;
    vector<Wave> mWave;
    vector<vPoint> vPs;
    voronoi_diagram<double> vD;
    Exporter mExp;
    fs::path assetDir;
    
    int mFpb        = 1920;   //48kHz/25fps = 1920
    //int start_sec   = 120 + 20;
    int start_sec   = 60*6 + 50;

    int frame       = -1;// + start_sec/0.040;
    float camDist   = -2800;
    float amp       = 600;
    int resolution  = 1;
    int totalMovFrame;
    int xoffset = 120;
    
    mtSvg guide;
    bool bInit = false;
    
    vector<vector<Colorf>> mColorSample1;
};

void cApp::setup(){
    
    setFrameRate(60);
    setWindowSize( win_w*0.5, win_h*0.5 );
    setWindowPos( 0, 0 );
    gl::enableVerticalSync();
    assetDir = mt::getAssetPath();
    guide.load( assetDir/"svg"/"n5_guide.svg" );
    
    mt::loadColorSample("img/colorSample/n5pickCol_1.png", mColorSample1 );
    
    Wave w1;  w1.create( assetDir/"snd"/"timeline"/"n5_6ch"/"n5_base.aif" );
    Wave w2;  w2.create( assetDir/"snd"/"timeline"/"n5_6ch"/"n5_event.aif" );
    Wave w3;  w3.create( assetDir/"snd"/"timeline"/"n5_6ch"/"n5_sine.aif" );
    Wave w4;  w4.create( assetDir/"snd"/"timeline"/"n5_2mix.aif" );
   
    mWave.push_back( w1 );
    mWave.push_back( w2 );
    mWave.push_back( w3 );
    mWave.push_back( w4 );
    
    Perlin pln1(4, 1835);
    Perlin pln2(4, 5244);
    Perlin pln3(4, 7491);
    Perlin pln4(4, 7491);

    mPlns.push_back( pln1 );
    mPlns.push_back( pln2 );
    mPlns.push_back( pln3 );
    mPlns.push_back( pln4 );
    
    int nFrame = mWave[0].buf->getNumFrames();
    totalMovFrame = nFrame / mFpb;
    
    mExp.setup( win_w, win_h, 0, totalMovFrame, GL_RGB, mt::getRenderPath(), 0 );
    
#ifdef RENDER
    mExp.startRender();
#endif

}

void cApp::update(){

    frame++;
    
    int nFrame = mWave[0].buf->getNumFrames();
    long audioPos = frame * mFpb;
    
    if( audioPos >= nFrame ){
        cout << "Quit App, frame: " << frame << "\n"
        << "audio pos: " << audioPos << ", audio total frame : " << nFrame << endl;
        quit();
    }
    

    vPs.clear();
    
    float canvasWidth = mExp.mFbo.getWidth()-xoffset*2;
    int sampleNum = mFpb/resolution;
    float sampleDistance = (float)canvasWidth/sampleNum;

    int waveId = -1;

    for( int w=0; w<mWave.size(); w++ ){
        
        for (int ch=0; ch<mWave[w].nCh; ch++) {
            
            waveId++;
            
            const float * data = mWave[w].buf->getChannel( ch );
            
            for ( int i=0; i<sampleNum+1; i++) {
            
                float l = data[audioPos + i];
                //float minus = l/abs(l);
                //l = log10(1+abs(l)*100) / 2.0f;
                //l *= minus;
                
                float x1 = i * sampleDistance;
                x1 -= mExp.mFbo.getWidth()/2;
                x1 += xoffset;
                x1 += sampleDistance/2 * (float)(w%2);
                
                float y1 = l * amp;
                y1 *= (w+2)*0.4;
                
                Vec3f n1 = mPlns[w].dfBm( frame*0.1*w, waveId*i*0.2, 1+abs(l)*0.5);
                x1 += n1.x*n1.z*2;
                y1 += n1.y*n1.x*2;
                
                float yoffset = 16;
                y1 -= yoffset*(mWave.size()*2-1)/2;
                y1 += waveId * yoffset;
                
                vPs.push_back( vPoint(x1, y1) );
            }
        }
    }
    
    vD.clear();
    boost::polygon::construct_voronoi( vPs.begin(), vPs.end(), &vD);
}

void cApp::draw(){

    gl::enableAlphaBlending();
    
    mExp.beginOrtho(); {
        gl::clear( Color(0,0,0) );
        glPushMatrix();
        gl::translate(mExp.mFbo.getWidth()/2, mExp.mFbo.getHeight()/2);
        draw_voronoi();
        glPopMatrix();
        
        if( !bInit ){
            guide.draw();
            bInit = true;
        }
        
    } mExp.end();

    gl::clear( Color(0,0,0) );
    glColor3f(1, 1, 1);
    mExp.draw();
    
}

void cApp::draw_voronoi(){

    if( 1 ){
        gl::lineWidth(1);
        gl::begin(GL_LINES);
        voronoi_diagram<double>::const_cell_iterator it = vD.cells().begin();
        for (; it!= vD.cells().end(); ++it) {
            
            const cell_type& cell = *it;
            const edge_type* edge = cell.incident_edge();
            
            do{
                if(edge->is_primary()){
                    if( edge->is_finite() ){
                        if (edge->cell()->source_index() < edge->twin()->cell()->source_index()){
                            float x0 = edge->vertex0()->x();
                            float y0 = edge->vertex0()->y();
                            float x1 = edge->vertex1()->x();
                            float y1 = edge->vertex1()->y();
                            
                            float limity = 800;
                            float limitx = 4000;
                            bool xOver = ( abs(x0)>limitx || abs(x1)>limitx );
                            bool yOver = ( abs(y0)>limity || abs(y1)>limity );
                            
                            bool draw = !xOver && !yOver;
              
                            Vec3f n = mPlns[0].dfBm(frame, x0, y1);
                            n.x = abs(n.x);
                            n.y = abs(n.y);
                            n.x *= mColorSample1[0].size()-1;
                            n.y *= mColorSample1.size()-1;
                            Colorf & col = mColorSample1[n.x][n.y];
                            glColor4fv(col);
                            
                            if(draw){
                                glVertex3f(x0, y0, 0);
                                glVertex3f(x1, y1, 0);
                            }else{
                                if( randFloat()>0.9){
                                    gl::color(0.5, 0.5, 0.5, 0.5);
                                    glVertex3f(x0, y0, 0);
                                    glVertex3f(x1, y1, 0);
                                }
                            }
                        }
                    }else{
                        if( 0 ){
                            const vertex_type * v0 = edge->vertex0();
                            if( v0 ){
                                vPoint p1 = vPs[edge->cell()->source_index()];
                                vPoint p2 = vPs[edge->twin()->cell()->source_index()];
                                float x0 = edge->vertex0()->x();
                                float y0 = edge->vertex0()->y();
                                float end_x = (p1.y() - p2.y()) * win_w;
                                float end_y = (p1.x() - p2.x()) * -win_w;
                                glVertex3f(x0, y0, 0);
                                glVertex3f(end_x, end_y, 0);
                            }
                        }
                    }
                }
                edge = edge->next();
            }while (edge != cell.incident_edge());
        }
        glEnd();
    }
    
    // draw Point
    if( 0 ){
        glPointSize(3);
        gl::color(1.0, 0.1, 0.1, 0.7);
        glBegin(GL_POINTS);
        for( auto v : vPs )
            glVertex3f( v.x(), v.y(), 0);
        glEnd();
    }
    

}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
