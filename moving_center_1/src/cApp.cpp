#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Xml.h"
#include "cinder/Timeline.h"

#include "Rail.h"
#include "ufUtil.h"

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
    
    vector<Rail> mRails;
    
    const float master_scale = 0.5;
    const int win_w = 4320;
    const int win_h = 1920;
    const float fps = 25.0f;
    TimelineRef mTimeline;
    
};

void cApp::setup(){
    setWindowSize( win_w*master_scale, win_h*master_scale );
    setWindowPos( 0, 0 );

    setFrameRate( fps );

    mTimeline = Timeline::create();
    
    XmlTree xml( loadAsset("rail_key_01.xml") );
    XmlTree seq = xml.getChild( "rail_sequence" );
    
    if( seq.hasChild("rail") ){
        
        XmlTree::Iter rail = seq.find("rail");

        for( int i=0; rail!=seq.end(); ++rail, i++ ) {
            
            mRails.push_back( Rail() );

            int id = fromString<int>( rail->getAttribute("id").getValue() );
            
            XmlTree::Iter key = rail->find( "key" );
            
            int prev_frame = 0;
            
            for ( int j=0; key!=rail->end(); ++key, j++ ) {
                
                int frame = fromString<int>( key->getChild("frame").getValue() );
                int x = fromString<int>( key->getChild("pos/x").getValue() );
                int y = fromString<int>( key->getChild("pos/y").getValue() );
                
                x *= master_scale;
                y *= master_scale;
                
                char m[255];
                sprintf(m, "Rail %2d, %3df: %4d, %4d", id, frame-prev_frame, x, y);
                cout << m << endl;
                
                Anim<Vec2f> * target = &mRails[i].pos;
                if( j==0 ){
                    target->value() = Vec2f(x,y);
                    timeline().apply(target, Vec2f(x,y), (frame-prev_frame)/fps );
                }else{
                    timeline().appendTo(target, Vec2f(x,y), (frame-prev_frame)/fps );
                }
                
                prev_frame = frame;
            }
        }
    }
    
    mTimeline->stepTo(0);
}

void cApp::update(){
    mTimeline->step(1.0/fps);
    
    int frame = mTimeline->getCurrentTime() * fps;
    if( frame%100 == 0){
        cout << frame << "f" << endl;
        
        for( auto r : mRails ){
            float x = r.pos.value().x;
            float y = r.pos.value().y;
            cout << x << "," <<  y << endl;
        }
    }
}

void cApp::draw(){

    gl::clear();
    
    gl::pushMatrices();
    gl::translate( getWindowWidth()/2, getWindowHeight()/2);
    uf::drawCoordinate(100);
    gl::popMatrices();
    
    glColor3f(1,0,0);
    glPointSize(5);
    glBegin( GL_POINTS );
    for( auto r : mRails ){
        float x = r.pos.value().x;
        float y = r.pos.value().y;
        glVertex2f( x, y );
    }
    glEnd();
    
    
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
