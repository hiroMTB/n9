#pragma once

#include "CinderOpenCv.h"

using namespace std;
using namespace ci;

class ContourMap{
    
public:
    
    ContourMap();
    
    void setImage( ci::Surface32f sur, bool convert2gray, cv::Size blurSize );
    void addContour( float threshold, int filterType=0, bool output_threshold_image=false );
    void drawContourGroup( int whichThreshold );
    void drawContourAll();
    void exportContour( string path, string fileType );
    
    typedef vector<cv::Point2f> Contour;              // 1 contour
    typedef vector<Contour> ContourGroup;           // 1 group of same threshold
    typedef vector<ContourGroup> ContourMapData;    // Countour Map data separated with threashold
    ContourMapData mCMapData;

    cv::Mat input;
    cv::Mat thresh;
};