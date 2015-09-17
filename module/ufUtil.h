#pragma once

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Vbo.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

typedef std::function<float (float)> EaseFunc;

namespace uf {
    
    gl::VboMesh::Layout getVboLayout(){
        gl::VboMesh::Layout layout;
        layout.setDynamicColorsRGBA();
        layout.setDynamicPositions();
        layout.setStaticIndices();
        return layout;
    }
    
    string getTimeStamp(){        
        time_t curr;
        tm local;
        time(&curr);
        local =*(localtime(&curr));
        int month = local.tm_mon;
        int day = local.tm_mday;
        int hours = local.tm_hour;
        int min = local.tm_min;
        int sec = local.tm_sec;
        
        char stamp[16];
        sprintf(stamp, "%02d%02d_%02d%02d_%02d", month+1, day, hours, min, sec);
        return string(stamp);
    }
    
    string getTimeStampU(){
        return toString( time(NULL) );
    }
    
    void renderScreen( fs::path path, int frame ){
        string frame_name = "f_" + toString( frame ) + ".png";
        writeImage( path/frame_name, copyWindowSurface() );
        cout << "Render Image : " << frame << endl;
    }
    
    fs::path getRenderPath( string subdir_name="" ){
        return expandPath("../../../_render") / getTimeStamp() / subdir_name ;
    }
    
    fs::path getProjectPath(){
        return expandPath("../../../");
    }
    
    float distanceToLine( const Ray &ray, const Vec3f &point){
        return cross(ray.getDirection(), point - ray.getOrigin()).length();
    }
    
    Vec3f dirToLine( const Vec3f &p0, const Vec3f &p1, const Vec3f &p2 ){
        Vec3f v = p2-p1;
        Vec3f w = p0-p1;
        double b = v.dot(w) / v.dot(v);
        return -w + b * v;
    }
    
    void fillSurface( Surface16u & sur, const ColorAf & col){

        Surface16u::Iter itr = sur.getIter();
        while (itr.line() ) {
            while( itr.pixel()){
                //setColorToItr( itr, col );
                sur.setPixel(itr.getPos(), col);
            }
        }
    }
    
    void drawCoordinate( float length=100 ){
        glBegin( GL_LINES ); {
            glColor3f( 1, 0, 0 );
            glVertex3f( 0, 0, 0 );
            glVertex3f( length, 0, 0 );
            glColor3f( 0, 1, 0 );
            glVertex3f( 0, 0, 0 );
            glVertex3f( 0, length, 0 );
            glColor3f(  0, 0, 1 );
            glVertex3f( 0, 0, 0 );
            glVertex3f( 0, 0, length );
        } glEnd();
    }
  
    
    void drawScreenGuide(){

        float w = getWindowWidth();
        float h = getWindowHeight();
        gl::pushMatrices();
        gl::setMatricesWindow( getWindowSize() );
        glBegin( GL_LINES ); {
            glColor3f( 1,1,1 );
            glVertex3f( w*0.5, 0, 0 );
            glVertex3f( w*0.5, h, 0 );
            glVertex3f( 0, h*0.5, 0 );
            glVertex3f( w, h*0.5, 0 );
        } glEnd();
        gl::popMatrices();
    }


    void loadColorSample( string fileName, vector<vector<Colorf>>& col){
        Surface sur( loadImage( loadAsset(fileName) ) );
        Surface::Iter itr = sur.getIter();
        
        int w = sur.getWidth();
        int h = sur.getHeight();
        
        col.assign( w, vector<Colorf>(h) );
        
        while ( itr.line() ) {
            
            while ( itr.pixel() ) {
                float r = itr.r() / 255.0f;
                float g = itr.g() / 255.0f;
                float b = itr.b() / 255.0f;
                
                Vec2i pos = itr.getPos();
                col[pos.x][pos.y].set( r, g, b );
            }
        }
        cout << "Load Success: " << col.size() << " ColorSample" << endl;
    }
}