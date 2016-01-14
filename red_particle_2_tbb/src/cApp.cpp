//#define RENDER
//#define PROXY
//#define REC_SIM
//#define CALC_3D

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Perlin.h"
#include "cinder/Rand.h"
#include "cinder/MayaCamUI.h"

#include "tbb/tbb.h"

#include "Exporter.h"
#include "mtUtil.h"
#include "VboSet.h"
#include "RfExporterBin.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace tbb;

class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    void task(float i );
    
    void keyDown( KeyEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    
    int mW;
    int mH;
    bool bOrtho = false;
    bool bTbb   = true;
    float dispScale = 0.5;
    float frame    = 0;
    Vec2f noiseCenter;
    fs::path assetDir = mt::getAssetPath();

    vector<Vec3f> spd;
    vector<float> mass;
    vector<Perlin> mPln;
    vector<Vec2i> absPos = { Vec2i(1640,192), Vec2i(2680,192), Vec2i(1640,1728), Vec2i(2680,1728) };

    VboSet vbo;
    Exporter mExp;
    
    
};

void cApp::setup(){

#ifdef PROXY
    mW = 1080*4/2;
    mH = 1920/2;
    setWindowSize( mW, mH );
#else
    mW = 1080*4;
    mH = 1920;
    setWindowSize( mW*dispScale, mH*dispScale );
#endif

    setWindowPos( 0, 0 );
    mExp.setup( mW, mH, 0, 1000-1, GL_RGB, mt::getRenderPath(), 0);
    
    int type = 1;
    switch( type ){
        case 0:
            mPln.push_back( Perlin(4, 132051) );
            noiseCenter.x = 1799;
            noiseCenter.y =  343;
            break;
        case 1:
            mPln.push_back( Perlin(4, 77666) );
            noiseCenter.x = 1639;
            noiseCenter.y = 1046;
            break;
        case 2:
            break;
            
        case 3:
            break;
    }
    
    noiseCenter.x /= dispScale;
    noiseCenter.y /= dispScale;

//#define SEARCH_MODE
#ifdef SEARCH_MODE
    noiseCenter.x = mW/2;
    noiseCenter.y = mH/2;
#endif
    
    int cnt = 0;
    
    Surface32f img = Surface32f( loadImage( assetDir/"img"/"redCloud"/"redCloud01.png" ));
    int iW = img.getWidth();
    int iH = img.getHeight();
    
    vbo.addPos( Vec3f(0,0,2000) );
    vbo.addPos( Vec3f(0,mH,2000) );
    vbo.addPos( Vec3f(mW,0,2000) );
    vbo.addPos( Vec3f(mW,mH,2000) );

    for( int i=0; i<4; i++){
        vbo.addCol( ColorAf(9,0,1,1) );
        spd.push_back( Vec3f(0,0,0) );
        mass.push_back( 1 );
    }

    
    for (int y=0; y<iH; y+=20) {
        for (int x=0; x<iW; x+=20) {

            Colorf col = img.getPixel( Vec2i(x, y) );
            if(col.r>0.25 ){
                Vec3f p(x, y, 0 );
                p.x += randFloat(-1.1)*mH*0.05;
                p.y += randFloat(-1,1)*mH*0.05;
#ifdef CALC_3D
                p.z += randFloat(-1,1)*100;
#else
                p.z = 0;
#endif
                
                p.y *=1.75;
                p.y -= 500;
                
                p.x += noiseCenter.x-iW/2;
                p.y += noiseCenter.y-iH/2;
                vbo.addPos( p );
                vbo.addCol( ColorAf(1,0,0, 0.8+col.r*0.1) );
                
                Vec3f vel = mPln[0].dfBm(x*0.005, y*0.005, 0 );
                spd.push_back( vel );
                mass.push_back( 1 );
                cnt++;
            }
        }
    }
    vbo.init( GL_POINTS, true, true );
    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){
    
    int loadNum = vbo.getPos().size(); // * MIN(frame/100.0, 1.0);
    int substep = 100;
    if( bTbb ){
        for( int i=0; i<substep; i++){
            parallel_for( blocked_range<size_t>(0, loadNum), [&](const blocked_range<size_t>& r) {
                for( size_t i=r.begin(); i!=r.end(); ++i ){
                    task( i );
                }
            });
        }
        
        /*
         *   parallel_for with 2D range, no difference for performance
         */
        /*
        parallel_for( blocked_range2d<size_t>(0, substep, 0, loadNum), [&](const blocked_range2d<size_t>& r) {
            for( size_t j=r.rows().begin(); j!=r.rows().end(); ++j ){
                for( size_t i=r.cols().begin(); i!=r.cols().end(); ++i ){
                    task( i );
                }
            }
        });
        */
        
    }else{
        
        for( int s=0; s<loadNum; s++){
            task( s );
        }
    }

#ifdef REC_SIM
    float rfScale = 0.05;   // 1/20
    RfExporterBin rfb;
    stringstream fileName;
    fileName << "red2_" << std::setw(5) << setfill('0') << frame << ".bin";
    
    Vec3f offset(mW/2-noiseCenter.x, mH/2-noiseCenter.y, 0);
    
    vector<float> rfpos, rfvel;
    for( int i=0; i<vbo.getPos().size(); i++ ){
        const Vec3f & p = vbo.getPos()[i];
        rfpos.push_back( (p.x+offset.x) * rfScale );
        rfpos.push_back( (p.y+offset.y) * rfScale );
        rfpos.push_back( (p.z+offset.z) * rfScale );

        const Vec3f & v = spd[i];
        rfvel.push_back(v.x * rfScale);
        rfvel.push_back(v.y * rfScale);
        rfvel.push_back(v.z * rfScale);
    }
    printf( "start writting RFbin : %s\n", fileName.str().c_str() );
    rfb.write( fileName.str(), rfpos, rfvel, mass );
#endif
    
    vbo.updateVboPos();
    vbo.updateVboCol();
}


void cApp::task( float i  ){
    
    float ox = 6  + randFloat(-1,1)*0.01;
    float oy = 4.2 + randFloat(-1,1)*0.01;
    
    float resRate = 0.2;
    float rate = (frame+1)/300.0;
    float nRate = rate + 0.2;
    if(nRate>1){
        nRate = pow( nRate, 2);
        nRate = MIN( nRate, 1000);
    }
    
    float oRate = MIN(nRate, 1.0);
    
    Vec3f p = vbo.getPos()[i];
    
    float noisePoint = 2.101;
    
    Vec3f n = mPln[0].dfBm( ox+p.x*0.001*resRate, oy+p.y*0.001*resRate, noisePoint );
    
    spd[i].x = spd[i].x * (1.0-oRate) + n.x*nRate;
    spd[i].y = spd[i].y * (1.0-oRate) + n.y*nRate;
    
    p.x += spd[i].x;
    p.y += spd[i].y;
   
#ifdef CALC_3D
    spd[i].z = spd[i].z * (1.0-oRate) + n.z*rate;
    p.z += spd[i].z;
#endif

    vbo.writePos(i, p);

}

void cApp::draw(){
    
    bOrtho ? mExp.beginOrtho() : mExp.beginPersp(58, 1, 100000); {

        gl::enableAlphaBlending();
        
        glPointSize(1);
        gl::clear( Colorf(0,0,0) );
        gl::pushMatrices();
        
        glTranslatef( mW/2-noiseCenter.x, mH/2-noiseCenter.y, 0);
        vbo.draw();

        glTranslatef( 0.1, 0.1, 0 );
        vbo.draw();

        gl::popMatrices();

    } mExp.end();
    
    mExp.draw();
    
    gl::pushMatrices();
    glTranslatef( mW/2, mH/2, 0);
    mt::drawCoordinate();
    mt::drawScreenGuide();
    gl::popMatrices();
    
    gl::color(0, 0, 0);
    gl::drawString( "fps : "+to_string(getAverageFps()) , Vec2i(20,20));
    gl::drawString( "frame : "+to_string(getElapsedFrames()) , Vec2i(20,40));
    gl::drawString( "nParticle : "+to_string(vbo.getPos().size()) , Vec2i(20,60));
    
    frame++;
}

void cApp::keyDown( KeyEvent event ){
    switch (event.getChar()) {
        case 's':   mExp.snapShot();          break;
        case 'S':   mExp.startRender();       break;
        case 'T':   mExp.stopRender();        break;
        case 'f':   frame+=100;               break;
        case 'o':   bOrtho = !bOrtho;         break;
        case 'p':   bTbb = !bTbb;         break;
    }
}

void cApp::mouseDown( MouseEvent event ){
    cout << event.getPos() << endl;
}

void cApp::mouseDrag( MouseEvent event ){
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
