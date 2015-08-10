#pragma once

#include "cinder/app/AppNative.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/gl/Vbo.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DataGroup{
    
public:
    
    DataGroup(){};
    
    void createDot( const vector<Vec3f> &v, const vector<ColorAf> &c, float aThreshold ){
        
        mThreshold = aThreshold;
        
        gl::VboMesh::Layout layout;
        layout.setStaticIndices();
        layout.setDynamicColorsRGBA();
        layout.setDynamicPositions();

        mDot.reset();
        mDot = gl::VboMesh( v.size(), 0, layout, GL_POINTS );

        gl::VboMesh::VertexIter itr( mDot );
        for(int i=0; i<mDot.getNumVertices(); i++){
            itr.setPosition( v[i] );
            itr.setColorRGBA( c[i] );
            ++itr;
        }
        
        //char m[255];
        //sprintf(m, "create vbo DOT    threshold %0.4f  %10d verts", mThreshold, mDot.getNumVertices() );
        //cout << m << endl;
    }
    

    void createLine( const vector<Vec3f> &v, const vector<ColorAf> &c, int num, float minDist=1, float maxDist=100 ){
    
        gl::VboMesh::Layout layout;
        layout.setStaticIndices();
        layout.setDynamicColorsRGBA();
        layout.setDynamicPositions();

        vector<Vec3f> sv;
        vector<ColorAf> sc;
        
        int id1;
        
        for( int i=0; i<num; i++ ){
            
            id1 = id1 + randInt(0, v.size()/num );
            id1 %= v.size();
            
            Vec3f v1 = v[id1];
            
            for( int j=0; j<50;j ++ ){
            
                int id2 = randInt(0, v.size() );
                Vec3f v2 = v[id2];
                
                float dist = v1.distance(v2);
                if( minDist<dist && dist<maxDist ){
                    sv.push_back( v1 );
                    sv.push_back( v2 );
                    sc.push_back( c[id1] );
                    sc.push_back( c[id2] );
                    
                    for( int k=0; k<3; k++ ){
                        sv.push_back( v1 + Vec3f(randFloat(), randFloat(), randFloat())*2.0 );
                        sv.push_back( v2 + Vec3f(randFloat(), randFloat(), randFloat())*2.0 );
                        sc.push_back( c[id1] );
                        sc.push_back( c[id2] );
                    }
                }
            }
        }

        mLine.reset();
        mLine = gl::VboMesh( sv.size(), 0, layout, GL_LINES );
        gl::VboMesh::VertexIter itr( mLine );
        
        for( int i=0; i<sv.size(); i++ ){
            itr.setPosition( sv[i] );
            
            ColorAf c = sc[i];
            c.a *= 0.33;
            itr.setColorRGBA( c );
            ++itr;
        }
        
//        char m[255];
//        sprintf(m, "create vbo Line   threshold %0.4f  %10d lines", mThreshold, mLine.getNumVertices()/2 );
//        cout << m << endl;

    }
    
    void addLine( const vector<Vec3f> &v, const vector<ColorAf> &c ){
        gl::VboMesh::Layout layout;
        layout.setStaticIndices();
        layout.setDynamicColorsRGBA();
        layout.setDynamicPositions();
        
        mLine.reset();
        mLine = gl::VboMesh( v.size(), 0, layout, GL_LINES );
        gl::VboMesh::VertexIter itr( mLine );
        
        for( int i=0; i<v.size(); i++ ){
            itr.setPosition( v[i] );
            
            itr.setColorRGBA( c[i] );
            ++itr;
        }
    }
    
    
    gl::VboMesh mDot;
    gl::VboMesh mLine;
    float       mThreshold;
    
};