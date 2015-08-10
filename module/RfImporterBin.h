#pragma once

/*
 *      Import binary file for RF particle simulation
 *      No framework needed.
 */

#include <stdio.h>
#include <iostream>
#include <vector>

using namespace std;

class RfImporterBin{
    
public:
    
    // grobal data
    int     gVerificationCode;
    char    gFluidName[250];
    short   gVersion;
    float   gScale;
    int     gFluidType;
    float   gElapTime;
    int     gFrameNum;
    int     gFps;
    int     gNumParticles;
    float   gRadius;
    float   gPressure[3];
    float   gSpeed[3];
    float   gTemperature[3];
    float   gEmPosition[3];
    float   gEmRotation[3];
    float   gEmScale[3];
    
    // per particle data
    vector<float> pPosition;
    vector<float> pVelocity;
    vector<float> pForce;
    vector<float> pVorticity;
    vector<float> pNormal;
    vector<int>   pNumNeihbors;
    vector<float> pTexVec;
    vector<short> pInfoBit;
    vector<float> pElapsedTime;
    vector<float> pIsoTime;
    vector<float> pViscosity;
    vector<float> pDensity;
    vector<float> pPressure;
    vector<float> pMass;
    vector<float> pTemperature;
    vector<int> pId;

    void load( string fileName ){
        
        FILE * pFile;
        
        pFile = fopen( fileName.c_str(), "rb");
        fread( &gVerificationCode,  sizeof(int),        1,   pFile );
        fread( gFluidName,          sizeof(char),     250,   pFile );
        fread( &gVersion,           sizeof(short),      1,   pFile );
        fread( &gScale,             sizeof(float),      1,   pFile );
        fread( &gFluidType,         sizeof(int),        1,   pFile );
        fread( &gElapTime,          sizeof(float),      1,   pFile );
        fread( &gFrameNum,          sizeof(int),        1,   pFile );
        fread( &gFps,               sizeof(int),        1,   pFile );
        fread( &gNumParticles,      sizeof(int),        1,   pFile );
        fread( &gRadius,            sizeof(float),      1,   pFile );
        fread( &gPressure,          sizeof(float),      3,   pFile );
        fread( &gSpeed,             sizeof(float),      3,   pFile );
        fread( &gTemperature,       sizeof(float),      3,   pFile );
        fread( &gEmPosition,        sizeof(float),      3,   pFile );
        fread( &gEmRotation,        sizeof(float),      3,   pFile );
        fread( &gEmScale,           sizeof(float),      3,   pFile );
        
        pPosition.clear();
        pVelocity.clear();
        pForce.clear();
        pVorticity.clear();
        pNormal.clear();
        pNumNeihbors.clear();
        pTexVec.clear();
        pInfoBit.clear();
        pElapsedTime.clear();
        pIsoTime.clear();
        pViscosity.clear();
        pDensity.clear();
        pPressure.clear();
        pMass.clear();
        pTemperature.clear();
        pId.clear();
        
        // Per Particle Data
        for( int i=0; i<gNumParticles; i++ ){
            float position[3];
            float velocity[3];
            float force[3];
            float vorticity[3];
            float normal[3];
            int   numNeihbors;
            float texVec[3];
            short infoBit;
            float elapsedTime;
            float isoTime;
            float viscosity;
            float density;
            float pressure;
            float mass;
            float temperature;
            int   id;
            
            fread( position,      sizeof(float),      3,  pFile );
            fread( velocity,      sizeof(float),      3,  pFile );
            fread( force,         sizeof(float),      3,  pFile );
            fread( vorticity,     sizeof(float),      3,  pFile );
            fread( normal,        sizeof(float),      3,  pFile );
            fread( &numNeihbors,  sizeof(int),        1,  pFile );
            fread( &texVec,        sizeof(float),      3,  pFile );
            fread( &infoBit,      sizeof(short),      1,  pFile );
            fread( &elapsedTime,  sizeof(float),      1,  pFile );
            fread( &isoTime,      sizeof(float),      1,  pFile );
            fread( &viscosity,    sizeof(float),      1,  pFile );
            fread( &density,      sizeof(float),      1,  pFile );
            fread( &pressure,     sizeof(float),      1,  pFile );
            fread( &mass,         sizeof(float),      1,  pFile );
            fread( &temperature,  sizeof(float),      1,  pFile );
            fread( &id,           sizeof(int),        1,  pFile );
            
            pPosition.push_back( position[0] );
            pPosition.push_back( position[1] );
            pPosition.push_back( position[2] );
            
            pVelocity.push_back( velocity[0] );
            pVelocity.push_back( velocity[1] );
            pVelocity.push_back( velocity[2] );

            pForce.push_back( velocity[0] );
            pForce.push_back( velocity[1] );
            pForce.push_back( velocity[2] );
            
            pVorticity.push_back( vorticity[0] );
            pVorticity.push_back( vorticity[1] );
            pVorticity.push_back( vorticity[2] );
            
            pNormal.push_back( normal[0] );
            pNormal.push_back( normal[1] );
            pNormal.push_back( normal[2] );
            
            pNumNeihbors.push_back( numNeihbors );
            
            pTexVec.push_back( texVec[0] );
            pTexVec.push_back( texVec[1] );
            pTexVec.push_back( texVec[2] );
            
            pInfoBit.push_back( infoBit );
            pElapsedTime.push_back( elapsedTime );
            pIsoTime.push_back( isoTime );
            pViscosity.push_back( viscosity );
            pDensity.push_back( density );
            pPressure.push_back( pressure );
            pMass.push_back( mass );
            pTemperature.push_back( temperature );
            pId.push_back( id );
        }
        
        // Ignore Footer
        /*
        int additioanl_data_per_particle;
        bool RF4_internal_data;
        bool RF5_internal_data;
        int dummyInt;
        
        fread( &additioanl_data_per_particle, sizeof(int), 1, pFile );
        fread( &RF4_internal_data, sizeof(bool),       1, pFile );
        fread( &RF5_internal_data, sizeof(bool),       1, pFile );
        fread( &dummyInt,          sizeof(int),        1, pFile );
        */
  
        fclose(pFile);
    }
};