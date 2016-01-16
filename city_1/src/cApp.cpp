#define RENDER

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Rand.h"
#include "Exporter.h"
#include "mtUtil.h"
#include "cinder/Perlin.h"

#include "tbb/tbb.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace tbb;

class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    
};

const int mW = 4320; // 1080*4
const int mH = 1920;
const float mScale = 0.2;
const float bldSize = 20;
int frame = 1;
int nCol = mW/(bldSize);
int nRow = mH/(bldSize);
fs::path renderDir;
fs::path assetDir;
fs::path bldDir;
Surface surface;
Exporter mExp;
//string area = "Treptow-Koepenick";
string area = "Charlottenburg-Wilmersdorf";
//string area = "Friedrichshain-Kreuzberg";

namespace TbbOp{
  
    struct makeTile{
        
        void operator()( const blocked_range<int>& range ) const {
            
            for( int a=range.begin(); a!=range.end(); ++a ){
                
                int i= a / nCol;
                int j= a % nCol;
                
                int dirId = randInt(1, 101) + frame*10;
                int numFile = 100;
                
                fs::recursive_directory_iterator itr( bldDir/to_string(dirId));
                int fileId = randInt(0, numFile);
                for(int k=0; k<fileId; k++){
                    ++itr;
                }
                
                fs::path filePath = itr->path();
                ImageSourceRef imgref = loadImage(filePath);
                Surface sur(imgref);
                Surface::Iter sitr = sur.getIter();
                
                float imgw = sur.getWidth();
                float imgh = sur.getHeight();
                
                if(imgw<bldSize || imgh<bldSize ){
                    a = max(0, --a);
                    continue;
                }
                
                float scalex = bldSize/imgw;
                float scaley = bldSize/imgh;
                
                float gx = j * bldSize;
                float gy = i * bldSize;
                
                while( sitr.line() ){
                    while ( sitr.pixel() ) {
                        float r = sitr.r()/255.0f;
                        float g = sitr.g()/255.0f;
                        float b = sitr.b()/255.0f;
                        float lum = (r+g+b) * 0.333;
                        Vec2i pos = sitr.getPos();
                        
                        pos.x *= scalex;
                        pos.y *= scaley;
                        pos.x += gx;
                        pos.y += gy;


                        ColorAf color;
                        if( lum<0.2 ){
                            color = ColorAf(r,g,b,0.7);
                        }else{
                            color = ColorAf(lum,lum,lum,0.7);
                        }
                        ColorAf base = surface.getPixel(pos);
                        ColorAf mix = base*0.5 + color*0.5;
                        mix.a = 1;
                        surface.setPixel( pos, mix );
                    }
                }
            }
        }
    };
};


void cApp::setup(){
    setWindowPos( 0, 0 );
    setWindowSize( mW*mScale, mH*mScale );
    mExp.setup( mW, mH, 0, 2999, GL_RGB, mt::getRenderPath(), 0 );
    surface = Surface( mW, mH, false);

    assetDir = mt::getAssetPath();
    renderDir = mt::getRenderPath()/area;
    bldDir = assetDir/"img"/"geo"/"city"/area/"appearance";
}

void cApp::update(){

    TbbOp::makeTile makeTile;
    parallel_for(blocked_range<int>(0,nRow * nCol), makeTile);

#ifdef RENDER
    writeImage( renderDir/("f_"+to_string(frame)+".png"), surface );
#endif
    
    frame++;
}

void cApp::draw(){
    gl::clear( Colorf(0,0,0) );
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
