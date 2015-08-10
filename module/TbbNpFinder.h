#pragma once

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

using namespace tbb;

class TbbNpFinder{
    
public:
    
    void findNearestPoints( const Vec3f * input, Vec3f * output, ColorAf * in_colors, ColorAf * out_colors, size_t n, int num_line, int num_dupl ){
        NearestPoints np;
        np.input = input;
        np.output = output;
        np.in_colors = in_colors;
        np.out_colors = out_colors;
        
        np.total_size = n;
        np.num_line = num_line;
        np.num_dupl = num_dupl;
        parallel_for( blocked_range<int>(0,n), np );
    }
    
private:
    struct NearestPoints {
        const Vec3f *input;
        Vec3f *output;
        ColorAf * in_colors;
        ColorAf * out_colors;
        
        int total_size;
        int num_line;
        int num_dupl;
        
        // input = all point
        // output = all point * 10
        
        void operator()( const blocked_range<int>& range ) const {
            
            multimap<float, Vec3f> near_p;
            pair<float, Vec3f> pair1( 999999999999, Vec3f(-12345,0,0) );
            
            for( int i=range.begin(); i!=range.end(); ++i ){

                //if( randFloat() < 0.7 ) continue;

                const Vec3f &pos1 = input[i];
                
                near_p.clear();
                for( int line=0; line<num_line; line++ ){
                    near_p.insert( pair1 );
                }
                
                //int try_num = 6000;
                int try_num = total_size;
                float fLimit = 1000000;
                float nLimit = randFloat(0.1f, 5.0f);

                for( int j=i+1; j<try_num; j++ ){
                    
                    int id2 = j;
                    if( i==id2 ) continue;
                    
                    const Vec3f &pos2 = input[id2];
                    
                    float sat_dist = 2;
                    int sat_num = 0;
                    
                    float dist = pos1.distance( pos2 );
                    if( nLimit<dist && dist<fLimit ){
                        multimap<float, Vec3f>::iterator itr = near_p.end();
                        
                        itr--;
                        if( dist < itr->first ){
                            std::pair<float, Vec3f> pair2( dist, pos2 );
                            near_p.insert( pair2 );
                            
                            multimap<float, Vec3f>::iterator end = near_p.end();
                            near_p.erase( --end );
                        }
                        
                        if( dist < sat_dist ){
                            if( ++sat_num >= num_line*2 ){
                                break;
                            }
                        }
                    }
                }
                
                multimap<float, Vec3f>::iterator itr = near_p.begin();
                ColorAf c = in_colors[i];
                for(int j=0; itr!=near_p.end(); itr++, j++ ){
                    
                    Vec3f &p2 = itr->second;

                    if( p2.x != -12345){
                        int outid = i*num_line*num_dupl*2 + j*num_dupl*2;
                        
                        output[outid] = pos1;
                        output[outid+1] = p2;
                        out_colors[outid] = c;
                        out_colors[outid+1] = c;
                        
                        for( int k=0; k<num_dupl-1; k++ ){
                            float rate = 1.2f;
                            Vec3f r1( randFloat(), randFloat(), randFloat() );
                            Vec3f r2( randFloat(), randFloat(), randFloat() );
                            
                            int did = outid + 2 + k*2;
                            output[ did ] = pos1 + r1*rate;
                            output[ did + 1] = p2 + r2*rate;

                            ColorAf cc = in_colors[i];
                            cc.r += r1.x*0.1;
                            cc.g += r1.y*0.1;
                            cc.b += r1.z*0.1;
                            cc.a *= 0.06;
                            out_colors[ did ] = cc;
                            out_colors[ did + 1] = cc;
                        }
                    }
                }
            }
        }
    };
    
};