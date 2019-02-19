// exported.h
#pragma once

// Define EXPORTED for cross-platform
#ifdef _WIN32
    #ifdef WIN_EXPORT
        // Exporting
        #ifdef __GNUC__
            #define EXPORTED __attribute__ ((dllexport))
        #else
            #define EXPORTED __declspec( dllexport )
        #endif
    #else
        #ifdef __GNUC__
            #define EXPORTED __attribute__ ((dllimport))
        #else
            #define EXPORTED __declspec( dllimport )
        #endif
    #endif
    #define NOT_EXPORTED
#else
    #if __GNUC__ >= 4
        #define EXPORTED __attribute__ ((visibility ("default")))
        #define NOT_EXPORTED  __attribute__ ((visibility ("hidden")))
    #else
        #define EXPORTED
        #define NOT_EXPORTED
    #endif
#endif
