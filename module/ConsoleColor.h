/*
 *
 *      Need to install XcodeColor plugin
 *      https://github.com/robbiehanson/XcodeColors
 *
 */

#pragma once

#include <iostream>
using namespace std;

namespace ccout{
    
#if TARGET_OS_IPHONE
    static const string escape  = "\xC2\xA0[";
#else
    static const string escape  = "\033[";
#endif
    
    static const string rstf     = escape + "fg;";   // reset fg
    static const string rstb     = escape + "bg;";   // reset bg
    static const string rst      = escape + ";";     // reset all
    
    static const string blue     = escape + "fg20,150,255;";
    static const string red     = escape + "fg255,50,50;";
    static const string gray   = escape + "fg100,100,100;";
    
    void b( const string &s ){
        cout << blue << s << rst << endl;
    }
    
    void r( const string &s ){
        cout << red << s << rst << endl;
    }
    
}

