#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "Exporter.h"
#include "mtUtil.h"

#include <boost/algorithm/string.hpp>
#include "VboSet.h"

using namespace ci;
using namespace ci::app;
using namespace std;


class KeyData{
    
public:
    KeyData( int _id, int _frame, float _val) : id(_id), val(_val), frame(_frame){}
    int id;
    float val;
    int frame;
};


class SpeedData{
    
public:
    int sFrame, eFrame;
    float sPos, ePos;
    float speed;
};

class AccelData{
    
public:
    int sFrame, eFrame;
    float sSpeed, eSpeed;
    float accel;
};

class cApp : public AppNative {
    
    
  public:
	void setup();
    void update();
    void draw();
    void keyDown( KeyEvent e );
    
    void loadCsv( fs::path path, vector<vector<KeyData>> & kd, vector<vector<KeyData>> & kd2, bool print=false );
    void calcPos();
    void calcOnoff();
    void calcSpeed();
    void calcAccel();
    
    void draw_rect();
    void drawRabel( string s );
    
    const double pixelPerM = 4320.0/18.0;
    const double mmPerPixel = 18.0 / 4320.0;
    const string csv_name = "n5_timeline_v9.3.1.csv";   // delimiter=

    double toM( double pix ){
        return mmPerPixel * pix;
    }
    
    double toPixel( double m ){
        return pixelPerM * m;
    }
    
    Exporter mExp;
    const int mW = 2500;
    const int mH = 1080;
    const float mScale = 1;
    
    int frame = 0;
    bool bAllSaveMode = false;
    
    fs::path assetDir;
    
    vector<Vec2i> leftpos{
        Vec2i(480,192), Vec2i(840,576), Vec2i(120,960), Vec2i(840,1344), Vec2i(480,1728),       // L
        Vec2i(2640,192), Vec2i(2280,576), Vec2i(3000,960), Vec2i(2280,1344), Vec2i(2640,1728)   // R
    };

    vector<vector<KeyData>> posKeys;
    vector<vector<KeyData>> onoffKeys;

    vector<vector<SpeedData>> spdData;
    vector<vector<AccelData>> aclData;

    vector<VboSet> vbos_pos;
    vector<VboSet> vbos_onoff;
    vector<VboSet> vbos_spd;
    vector<VboSet> vbos_acl;
    
    vector<int> activeFrame;
    vector<pair<int, float>> maxSpeeds;
    vector<pair<int, float>> maxAccels;
    
    float gWidth  = 12000/4; //mW * 0.9;
    float gHeight = mH * 0.15;
    
    int dispRail = 0;
    
    vector<string> railName = { "L1", "L2", "L3", "L4", "L5", "R1", "R2", "R3", "R4", "R5" };
    
    
    float graphMaxSpeed = 5.0f;
    float graphMaxAccel = 50.0f;

};

void cApp::setup(){
    setWindowPos( 0, 0 );
    setWindowSize( mW*mScale, mH*mScale );
    setFrameRate(25);
    assetDir = mt::getAssetPath();
    
    // data
    posKeys.assign(10, vector<KeyData>() );
    onoffKeys.assign(10, vector<KeyData>() );
    
    spdData.assign(10, vector<SpeedData>());
    aclData.assign(10, vector<AccelData>());
    
    activeFrame.assign(10, float() );
    maxSpeeds.assign(10, pair<int, float>(-123, numeric_limits<float>::min()) );
    maxAccels.assign(10, pair<int, float>(-123, numeric_limits<float>::min()) );
    
    // draw
    vbos_pos.assign( 10, VboSet() );
    vbos_onoff.assign( 10, VboSet() );
    vbos_spd.assign( 10, VboSet() );
    vbos_acl.assign( 10, VboSet() );
    
    mExp.setup( mW, mH, 0, 2, GL_RGB, mt::getRenderPath(), 0 );
    
    loadCsv( assetDir/"csv"/csv_name, posKeys, onoffKeys, false );

    calcPos();
    calcOnoff();
    calcSpeed();
    calcAccel();
    
}

void cApp::calcPos(){
    
    for( int i=0; i<posKeys.size(); i++){
        
        const vector<KeyData> & pk = posKeys[i];
        
        for( int j=0; j<pk.size()-1; j++){
            
            int frame1 = pk[j].frame;
            float val1 = pk[j].val;
            float xrate1 = (float)frame1/12000.0f;
            float yrate1 = (val1 - leftpos[i].x)/1200.0f;

            int frame2 = pk[j+1].frame;
            float val2 = pk[j+1].val;
            float xrate2 = (float)frame2/12000.0f;
            float yrate2 = (val2 - leftpos[i].x)/1200.0f;

            vbos_pos[i].addPos( Vec3f(gWidth*xrate1, -gHeight*yrate1, 0));
            vbos_pos[i].addPos( Vec3f(gWidth*xrate2, -gHeight*yrate2, 0));

            ColorAf col( 0,0,1,1);
            if(val1 == val2){
                col.set(0.6, 0.6, 0.6, 1);
            }
            
            vbos_pos[i].addCol( col );
            vbos_pos[i].addCol( col );
        }
    }

    for( int i=0; i<vbos_pos.size(); i++){
        vbos_pos[i].init( GL_LINES );
    }
}

void cApp::calcOnoff(){
    
    for( int i=0; i<onoffKeys.size(); i++){
        
        const vector<KeyData> & pk = onoffKeys[i];
        
        for( int j=0; j<pk.size(); j++){
            
            int frame = pk[j].frame;
            float val = pk[j].val;
            
            float xrate = (float)frame/12000.0f;
            float yrate;
            ColorAf col( 1, 0.2, 0.7, 1);    // pink
            if( j>0 ){
                
                // check previous frame to make ON/OFF square wave
                const KeyData & pkey = pk[j-1];
                if( pkey.val != val ){
                    float valp = val==0?255:0;
                    float xratep = (float)frame/12000.0f;
                    float yratep = valp/255.0f;
                    vbos_onoff[i].addPos( Vec3f(gWidth*xratep, -gHeight*yratep, 0));
                    vbos_onoff[i].addCol( col );
                }
            }
            yrate = val/255.0f;
            vbos_onoff[i].addPos( Vec3f(gWidth*xrate, -gHeight*yrate, 0));
            vbos_onoff[i].addCol( col );
        }
    }

    for( int i=0; i<vbos_onoff.size(); i++){
        vbos_onoff[i].init( GL_LINE_STRIP );
    }
}

void cApp::calcSpeed(){
    
    
    vector<vector<KeyData>> & keys = posKeys;
    
    for( int i=0; i<keys.size(); i++){
        const vector<KeyData> & keyvec = keys[i];
        vector<SpeedData> & mds = spdData[i];
        
        int & af = activeFrame[i];
        
        for( int j=0; j<keyvec.size()-1; j++){
            
            const KeyData & key1 = keyvec[j];
            const KeyData & key2 = keyvec[j+1];
            
            SpeedData m;
            m.sFrame = key1.frame;
            m.eFrame = key2.frame;
            m.sPos = key1.val;
            m.ePos = key2.val;
            
            float pix_dist = m.ePos - m.sPos;
            int dur_frame = m.eFrame - m.sFrame;
            float dur_sec = (float)dur_frame*0.040;
            m.speed = pix_dist / dur_sec;
            mds.push_back( m );
            
            float abs_speed = abs(m.speed);
            if(abs_speed > maxSpeeds[i].second ){
                maxSpeeds[i].first = m.sFrame;
                maxSpeeds[i].second = abs_speed;
            }
            
            bool isStop = abs(m.speed) == 0;
            if( !isStop ){
                af += dur_frame;
            }
        }
    }
    
    for (int i=0; i<activeFrame.size(); i++) {
        int af = activeFrame[i];
        printf("rail %d : active frame= %d, %0.3f%%\n", i, af, (float)af/12000.0f * 100.0f);
    }
    
    for (int i=0; i<maxSpeeds.size(); i++) {
        int f = maxSpeeds[i].first;
        float speed = maxSpeeds[i].second;
        
        printf("rail %d : Max speed = %0.3f m/s at %d frame\n", i, toM(speed), f);
    }
    
    for( int i=0; i<spdData.size(); i++){
        vector<SpeedData> & mds = spdData[i];
        VboSet & vbo = vbos_spd[i];
        
        int maxSpeedFrame = maxSpeeds[i].first;
        
        for( int j=0; j<mds.size(); j++){

            const SpeedData & m = mds[j];
            float xrate1 = (float)m.sFrame/12000.0f;
            float xrate2 = (float)m.eFrame/12000.0f;

            float maxSpeed = toPixel(graphMaxSpeed); //4320/2;  // 4320/2 pix = 9000mm
            float yrate1 = (float) -m.speed/(maxSpeed) * 0.5;
            
            Vec3f spd1( xrate1*gWidth, yrate1*gHeight - gHeight*0.5, 0);
            Vec3f spd2( xrate2*gWidth, yrate1*gHeight - gHeight*0.5, 0);
            
            vbo.addPos(spd1);
            vbo.addPos(spd2);
            
            ColorAf c(0,0.5,0,1);
            
            if( m.sFrame == maxSpeedFrame ){
                c.set(1,0,0,1);
            }
            vbo.addCol( c );
            vbo.addCol( c );
        }
        
        vbo.init( GL_LINE_STRIP );
    }
}

void cApp::calcAccel(){
    
    vector<vector<SpeedData>> & spdss = spdData;
    vector<vector<AccelData>> & aclss = aclData;
    
    for( int i=0; i<spdss.size(); i++){
        
        vector<SpeedData> & spds = spdss[i];
        vector<AccelData> & acls = aclss[i];
        
        for( int j=0; j<spds.size()-1; j++){
            
            SpeedData & sd1 = spds[j];
            SpeedData & sd2 = spds[j+1];


            /*
             *      データ的には0フレームでスピードが上がるようになっている。
             *      ここではスピードの変化に2フレーム, 80msec 必ずかかることにして計算する。
             */
            int sFrame = sd1.eFrame;        // speed change start frame
            int eFrame = sd2.sFrame + 2;    // spped change end frame
            float sSpeed = sd1.speed;
            float eSpeed = sd2.speed;
            int dur_frame = eFrame - sFrame;
            
            float dur_sec = dur_frame * 0.040f;
            float speed_change = eSpeed - sSpeed;
            float accel = speed_change / (float)dur_sec;
            
            AccelData acl;
            acl.sFrame = sFrame;
            acl.eFrame = eFrame;
            acl.sSpeed = sSpeed;
            acl.eSpeed = eSpeed;
            acl.accel = accel;
            acls.push_back(acl);
            
            float abs_accel = abs(acl.accel);
            if(abs_accel > maxAccels[i].second ){
                maxAccels[i].first = acl.sFrame;
                maxAccels[i].second = abs_accel;
            }

        }
    }
    
    for (int i=0; i<maxAccels.size(); i++) {
        int f = maxAccels[i].first;
        float acl = maxAccels[i].second;
        
        printf("rail %d : Max Accel = %0.3f m/ss at %d frame\n", i, toM(acl), f);
    }
    
    for( int i=0; i<aclss.size(); i++){
        vector<AccelData> & acls = aclss[i];
        VboSet & vbo = vbos_acl[i];
        
        for( int j=0; j<acls.size(); j++){
            const AccelData & a = acls[j];
            
            float xrate1 = (float)a.sFrame/12000.0f;
            float xrate2 = (float)a.eFrame/12000.0f;
            
            float maxAccel = toPixel(graphMaxAccel); //50.0f/18.0f*4320.0f;    // 50m/s2, 9599pix/s2
            float yrate1 = (float) -a.accel/maxAccel * 0.5;
            
            Vec3f acl1a( xrate1*gWidth, yrate1*gHeight - gHeight*0.5, 0);
            Vec3f acl1b( xrate1*gWidth, -gHeight*0.5, 0);
            
            Vec3f acl2a( xrate2*gWidth, yrate1*gHeight - gHeight*0.5, 0);
            Vec3f acl2b( xrate2*gWidth, -gHeight*0.5, 0);
            
            vbo.addPos(acl1a);
            vbo.addPos(acl1b);
            
            ColorAf c1(0,0,0,1);
            ColorAf c2(0,0,0,1);
            if(a.sFrame == maxAccels[i].first){
                c1.set(1,0,0,1);
            }
            
            vbo.addCol( c1 );
            vbo.addCol( c2 );
        }
        vbo.init(GL_LINES);
    }
}

void cApp::loadCsv( fs::path path, vector<vector<KeyData>> & posKeys,vector<vector<KeyData>> & onoffKeys, bool print ){

    ifstream file ( path.string() );
    if( !file.is_open() ){
        cout << "cant open file" << endl;
    }else{
        std::string line;
        int cnt = -1;
        while ( std::getline( file, line ) ){
            if(++cnt<3) continue;
            
            line.erase( std::remove(line.begin(), line.end(), '\r'), line.end() );  // erase \r

            vector<string> strs;
            boost::split(strs, line, boost::is_any_of(",") );
            if( strs[0]==""){
                break;
            }
            
            vector<string> smpte;
            boost::split( smpte, strs[0], boost::is_any_of(":") );
            int frame = fromString<int>(smpte[0])*60*25 + fromString<int>(smpte[1])*25 + fromString<int>(smpte[2]);
            //int msec = frame * 40;
            
            for( int i=0; i<10; i++ ){
                string pos =   strs[i*2+1];
                string onoff = strs[i*2+2];
                
                if( pos != "" ){
                    float posv = fromString<float>(pos);
                    posKeys[i].push_back( KeyData(i, frame, posv) );
                }
                
                if( onoff != "" ){
                    int onoffv = fromString<float>(onoff);
                    onoffKeys[i].push_back( KeyData(i, frame, onoffv) );
                }
            }
        }
    }
    
    
    // check key data and add last frame
    for ( auto &pk : posKeys){
        KeyData & tail = pk[pk.size()-1];
        if( tail.frame != 12000-1 ){
            KeyData last(tail.id, 12000-1, tail.val);
            pk.push_back( last );
        }
    }
    
    for ( auto &ok : onoffKeys){
        KeyData & tail = ok[ok.size()-1];
        if( tail.frame != 12000-1 ){
            KeyData last(tail.id, 12000-1, tail.val);
            ok.push_back( last );
        }
    }
    
    if( print ){
        for (auto & pk : posKeys ){
            for (auto & k : pk){
                printf( "%d %d, %f\n", k.id, k.frame, k.val);
            }
        }
    }
}

void cApp::update(){
    
    if(bAllSaveMode){

        if(dispRail>9){
            bAllSaveMode = false;
            dispRail=0;
            return;
        }
        
        mExp.snapShot( railName[dispRail]+".png");
    }
}

void cApp::draw_rect(){
    gl::color(0.9,0.9,0.9);
    gl::drawSolidRect( Rectf(0,0,gWidth,-gHeight) );
    gl::color(1,1,1);
    float sep = 8.0f;
    for( int i=1;i<sep; i++){
        gl::drawLine(Vec2f((float)i*gWidth/sep, 0), Vec2f((float)i*gWidth/sep, -gHeight) );
    }
}

void cApp::draw(){

    gl::enableAlphaBlending();
    
    mExp.beginOrtho();
    {
        gl::clear( Colorf(1,1,1) );
        
        glPointSize(1);
        glLineWidth(1);
        gl::pushMatrices();{
            
            int gap = 70;
            
            glTranslatef( (mW-gWidth)/2, 0, 0 );

            glTranslatef( 0, gap, 0 );
            drawRabel("Position: 0m - 5m");
            glTranslatef( 0, gHeight, 0 );

            draw_rect();
            vbos_pos[dispRail].draw();
            
            glTranslatef( 0, gap, 0 );
            drawRabel("Laser ON/OFF");
            glTranslatef( 0, gHeight, 0 );
            draw_rect();
            vbos_onoff[dispRail].draw();
            
            
            glTranslatef( 0, gap, 0 );
            drawRabel("Speed : -" + toString((int)graphMaxSpeed)+ "m - +" + toString((int)graphMaxSpeed) + "m");
            glTranslatef( 0, gHeight, 0 );

            draw_rect();
            gl::color(0.7, 0.7, 0.7);
            gl::drawLine(Vec2f(0, -gHeight/2), Vec2f(gWidth, -gHeight/2) );
            vbos_spd[dispRail].draw();

            glTranslatef( 0, gap, 0 );
            drawRabel("Acceleration : -" + toString((int)graphMaxAccel)+ "m - +"+ toString((int)graphMaxAccel) + "m");
            glTranslatef( 0, gHeight, 0 );
            
            draw_rect();
            gl::color(0.7, 0.7, 0.7);
            gl::drawLine(Vec2f(0, -gHeight/2), Vec2f(gWidth, -gHeight/2) );
            glLineWidth(1);
            vbos_acl[dispRail].draw();

            // info
            glTranslatef( 0, gap, 0 );
            gl::color(0,0,0,1);
            gl::drawSolidRect(Rectf(0,0,1000,30) );
            gl::drawString( "Axis: " + railName[dispRail], Vec2i(15,15) );
            gl::drawString( "Active frames: " + to_string(activeFrame[dispRail]) + "f / 12000f", Vec2i(215,15) );
            gl::drawString( "Duty : " + to_string(activeFrame[dispRail]/12000.0f*100.0f) + " %", Vec2i(415,15) );
            gl::drawString( "Max Speed : " + to_string( toM(maxSpeeds[dispRail].second)) + " m/s", Vec2i(615,15) );
            gl::drawString( "Max Accel : " + to_string( toM(maxAccels[dispRail].second)) + " m/s2", Vec2i(815,15) );
       
            gl::pushMatrices();
            glTranslatef( mW-(mW-gWidth)/2-50, 0, 0 );
            gl::drawSolidRect(Rectf(0,0,-50,30) );
            gl::drawString( "8 min", Vec2i(-50+10,15) );
            gl::color(0, 0, 0, 1);
            gl::drawLine(Vec2f(0,0), Vec2f(0,-gap));
            gl::popMatrices();
            
        }
        gl::popMatrices();

        glTranslatef( mW-150, 0, 0 );
        gl::color(0,0,0,1);
        gl::drawSolidRect(Rectf(0,0,150,20) );
        gl::drawString( csv_name, Vec2i(15,8) );

    }
    mExp.end();

    mExp.draw();
    
    if(bAllSaveMode) dispRail++;

}

void cApp::drawRabel( string s ){
    gl::color(0, 0, 0, 1);
    gl::drawSolidRect(Rectf(0,-3,200,-20) );
    gl::color(1, 1, 1, 1);
    gl::drawString(s, Vec2i(10,-12) );
}

void cApp::keyDown( KeyEvent e){

    // '0'=48
    int k = e.getCode();
    if( 48<=k && k<48+10)
        dispRail =  k -48;

    switch( e.getChar() ){
        case 's': mExp.snapShot(); break;
        case 'S': bAllSaveMode = true; dispRail=0; break;
    }
    
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
