//#define RENDER

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "Exporter.h"
#include "mtUtil.h"
#include "Axis.h"

#include <boost/algorithm/string.hpp>

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    
    void loadCsv( fs::path path, vector<vector<float>> & array, bool print=false );
    void setupFromBlender();
    
    Exporter mExp;
    const int mW = 4320; // 1080*4
    const int mH = 1920;
    const float mScale = 0.5;
    
    int frame = 0;
    
    fs::path assetDir;
    vector<n5::Axis> axis;
    
    vector<Vec2i> leftpos{
        Vec2i(480,192), Vec2i(840,576), Vec2i(120,960), Vec2i(840,1344), Vec2i(480,1728),       // L
        Vec2i(2640,192), Vec2i(2280,576), Vec2i(3000,960), Vec2i(2280,1344), Vec2i(2640,1728)   // R
    };
};

void cApp::setup(){
    setWindowPos( 0, 0 );
    setWindowSize( mW*mScale, mH*mScale );
    setFrameRate(25);
    assetDir = mt::getAssetPath();
    
    mExp.setup( mW, mH, 0, 2999, GL_RGB, mt::getRenderPath(), 0, true );
    
    setupFromBlender();
    
#ifdef RENDER
    mExp.startRender();
#endif

}


void cApp::setupFromBlender(){
    
    vector<vector<float>> csv;
    loadCsv( assetDir/"csv"/"n5_timeline_v6_export_csv", csv );

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

/*
       csv file format
       L1:x1, L1:laserOnoff, L2:x1, L2:laserOnoff, ..., R5:x1, R1:laserOnoff
       each line number = frame number (25fps)
 */
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

void cApp::update(){
    for( int i=0; i<axis.size(); i++){
        axis[i].update( frame );
        axis[i].check();
    }
}

void cApp::draw(){

    mExp.beginOrtho();
    {
        gl::clear( Colorf(0,0,0) );
        gl::color( Colorf(1,0,0) );
        
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
    mExp.end();

    mExp.draw();
    gl::color(1, 1, 1);
    gl::drawString("frame : " + to_string(frame), Vec2i(10,10) );
    
    frame++;
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
