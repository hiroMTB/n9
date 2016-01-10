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
    
    void loadCsv( fs::path path, vector<vector<string>> & array, bool print=false );
    void setupFromNumbers();
    void setupFromBlender();

    int toFrame( string smpte);
    
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
    
    mExp.setup( mW, mH, 2999, GL_RGB, mt::getRenderPath(), 0, true );
    
    setupFromBlender();
    
#ifdef RENDER
    mExp.startRender();
#endif

}


void cApp::setupFromBlender(){
    
    vector<vector<string>> csv;
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
            float xpos = fromString<float>( csv[i][j*2] );
            int power = fromString<int>( csv[i][j*2+1] );
            int y = axis[j].left.y;
            axis[j].animPos[i].x = xpos;
            axis[j].animPos[i].y = y;
            axis[j].animPower[i] = power;
        }
    }
}

void cApp::loadCsv( fs::path path, vector<vector<string>> & array, bool print ){

    ifstream file ( path.string() );
    if( !file.is_open() ){
        cout << "cant open file" << endl;
    }else{
        std::string line;
        while ( std::getline( file, line ) ){
            
            std::istringstream iss{ line };
            std::vector<std::string> tokens;
            std::string cell;
            // erase \r
            while (std::getline(iss, cell, ',')){
                cell.erase( std::remove(cell.begin(), cell.end(), '\r'), cell.end() );
                tokens.push_back( cell );
            }
            
            array.push_back( tokens );
        }
    }
    
    if( print ){
        for (auto & line : array){
            for (auto & val : line){
                printf( "%6s ", val.c_str());
            }
            printf("\n");
        }
    }
}

void cApp::update(){
    for( int i=0; i<axis.size(); i++){
        axis[i].update( frame );
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
            
            bool on = (ax.power>200);
            on ? glColor3f( 1,0,0 ) : glColor3f( 0.3,0.3,0.3 );
            if( on ) gl::drawStrokedCircle(ax.pos, 30);

            glBegin( GL_POINTS );
            glVertex2f( ax.pos );
            glEnd();
        }
        
        gl::color( Colorf(1,0,0) );
        gl::drawStrokedCircle(Vec2i(100,100), 20);
        
    }
    mExp.end();

    mExp.draw();
    gl::color(1, 1, 1);
    gl::drawString("frame : " + to_string(frame), Vec2i(10,10) );
    
    frame++;
}

void cApp::setupFromNumbers(){
    vector<vector<string>> csv;
    loadCsv( assetDir/"csv"/"n5_timeline_v6.csv", csv, true);
    
    for( int i=0; i<10; i++){
        string name = csv[0][i*2+1];
        int y = fromString<int>( csv[2][i*2+1] );
        int x = fromString<int>( csv[3][i*2+1] );
        int power = fromString<int>( csv[3][i*2+2] );
        
        n5::Axis ax;
        ax.name = name;
        ax.pos.x = x;
        ax.pos.y = y;
        ax.left = leftpos[i];
        printf("Axis %d : Name= %s, x=%d, y=%d, power=%d\n", i, name.c_str(), x, y, power );
        
        //
        //  make animation data for position
        //
        vector< tuple<int, Vec2i>> posKeys;
        for( int j=3; j<csv.size(); j++){
            
            string xCell = csv[j][i*2+1];
            
            if( xCell != "" ){
                
                int valX = fromString<int>( xCell );
                
                // calc frame
                string smpte = csv[j][0];
                int frame = toFrame( smpte );
                posKeys.push_back( tuple<int, Vec2i>(frame, Vec2i( y, valX) ) );
            }
        }
        
        //
        //  make animation data for laser power
        //
        ax.animPower.assign(12000, 0);
        unsigned int pastVal = fromString<int>( csv[3][i*2+2] );
        unsigned int prevFrame = 0;
        for( int j=3; j<csv.size(); j++){
            
            string smpte = csv[j][0];
            int frame = toFrame( smpte );
            
            // fill same val from past key
            //for( prevFrame -> this frame)
            
            string powCell = csv[j][i*2+2];
            unsigned int powVal = pastVal;
            
            if( powCell != ""){
                powVal = fromString<int>( powCell );
            }
            
            ax.animPower[frame] = powVal;
            if(j>3+12000)
                break;
        }
        
        axis.push_back( ax );
    }
    
}

//
//  (string)min:sec:frame -> (int)frame
//
int cApp::toFrame( string smpte){
    std::vector<std::string> m_s_f;
    boost::split(m_s_f, smpte, boost::is_any_of(":"));
    int min = fromString<int>(m_s_f[0]);
    int sec = fromString<int>(m_s_f[1]);
    int fr = fromString<int>(m_s_f[2]);
    int frame = min*60*25 + sec*25 + fr;
    return frame;
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
