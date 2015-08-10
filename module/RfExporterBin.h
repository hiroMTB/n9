#pragma once

/*
 *      Export binary file for RF particle simulation
 *      No framework needed.
 */

#include <stdio.h>
#include <iostream>
#include <vector>

using namespace std;

class RfExporterBin{
    
public:
    
    void write( string fileName, const vector<float> &position, const vector<float> &velocity ){

        FILE * pFile;
        int     verificationCode = 0xFABADA;
        char    fluidName[250]  = "myFluidFromC++";
        short   version         = 11;
        float   scale           = 1.0f;
        int     fluidType       = 8;
        float   elapTime        = 0.0f;
        int     frameNum        = 0;
        int     fps             = 30;
        int     numParticles    = position.size() / 3;
        float   radius          = 0.1f;
        float   pressure[3]     = { -1000, 2500, 2000 };
        float   speed[3]        = { 1, 2, 0 };
        float   temperature[3]  = { 300, 300, 299.9 };
        float   emPosition[3]   = { 0, 1, 0 };
        float   emRotation[3]   = { 0, 0, 0 };
        float   emScale[3]      = { 1,1,1 };
        
        // HEADER
        pFile = fopen ( fileName.c_str(), "wb");
        fwrite( &verificationCode,  sizeof(int),        1,   pFile );
        fwrite( fluidName,          sizeof(char),     250,   pFile );
        fwrite( &version,           sizeof(short),      1,   pFile );
        fwrite( &scale,             sizeof(float),      1,   pFile );
        fwrite( &fluidType,         sizeof(int),        1,   pFile );
        fwrite( &elapTime,          sizeof(float),      1,   pFile );
        fwrite( &frameNum,          sizeof(int),        1,   pFile );
        fwrite( &fps,               sizeof(int),        1,   pFile );
        fwrite( &numParticles,      sizeof(int),        1,   pFile );
        fwrite( &radius,            sizeof(float),      1,   pFile );
        fwrite( &pressure,          sizeof(float),      3,   pFile );
        fwrite( &speed,             sizeof(float),      3,   pFile );
        fwrite( &temperature,       sizeof(float),      3,   pFile );
        fwrite( &emPosition,        sizeof(float),      3,   pFile );
        fwrite( &emRotation,        sizeof(float),      3,   pFile );
        fwrite( &emScale,           sizeof(float),      3,   pFile );
        
        // Per Particle Data
        for( int i=0; i<numParticles; i++ ){
            
            float pPosition[3]   = { position[i*3+0],   position[i*3+1],    position[i*3+2] };
            float pVelocity[3]   = { velocity[i*3+0],   velocity[i*3+1],    velocity[i*3+2] };
            float pForce[3]      = { 0.0f, 0.0f, 0.0f };
            float pVorticity[3]  = { 0.0f, 0.0f, 0.0f };
            float pNormal[3]     = { 0.0f, 0.0f, 0.0f };
            int   pNumNeihbors   = 0;
            float pTexVec[3]     = { 0.0f, 0.0f, 0.0f };
            short pInfoBit       = 0;
            float pElapsedTime   = 0.0f;
            float pIsoTime       = 0.0f;
            float pViscosity     = 3.0f;
            float pDensity       = 539;
            float pPressure      = -1000.0f;
            float pMass          = 1.0f;
            float pTemperature   = 300.0f;
            int   pId            = numParticles - i;
            
            fwrite( pPosition,      sizeof(float),      3,  pFile );
            fwrite( pVelocity,      sizeof(float),      3,  pFile );
            fwrite( pForce,         sizeof(float),      3,  pFile );
            fwrite( pVorticity,     sizeof(float),      3,  pFile );
            fwrite( pNormal,        sizeof(float),      3,  pFile );
            fwrite( &pNumNeihbors,  sizeof(int),        1,  pFile );
            fwrite( pTexVec,        sizeof(float),      3,  pFile );
            fwrite( &pInfoBit,      sizeof(short),      1,  pFile );
            fwrite( &pElapsedTime,  sizeof(float),      1,  pFile );
            fwrite( &pIsoTime,      sizeof(float),      1,  pFile );
            fwrite( &pViscosity,    sizeof(float),      1,  pFile );
            fwrite( &pDensity,      sizeof(float),      1,  pFile );
            fwrite( &pPressure,     sizeof(float),      1,  pFile );
            fwrite( &pMass,         sizeof(float),      1,  pFile );
            fwrite( &pTemperature,  sizeof(float),      1,  pFile );
            fwrite( &pId,           sizeof(int),        1,  pFile );
        }
        
        // Footor
        int additioanl_data_per_particle = 0;
        bool RF4_internal_data = 0;
        bool RF5_internal_data = 1;
        int dummyInt = 0;
        
        fwrite( &additioanl_data_per_particle, sizeof(int), 1, pFile );
        fwrite( &RF4_internal_data, sizeof(bool),       1, pFile );
        fwrite( &RF5_internal_data, sizeof(bool),       1, pFile );
        fwrite( &dummyInt,          sizeof(int),        1, pFile );
        
        fclose(pFile);
    }
};