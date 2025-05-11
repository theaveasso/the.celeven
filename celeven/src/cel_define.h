#pragma once

#if defined(_WIN32)
    #if defined(__TINYC__)
        #define __declspec(x) __attribute__((x))
    #endif// __TINYC__

    #if defined(BUILD_SHARED)
        #define CELAPI __declspec(dllexport)
    #elif defined(USE_SHARED)
        #define CELAPI __declspec(dllimport)
    #endif// BUILD_SHARED
#else
    #if defined(BUILD_SHARED)
        #define CEL_API __attribute__(visibilitity("default")))
    #endif// BUILD_SHARED
#endif    // _WIN32

#if !defined(CELAPI)
    #define CELAPI
#endif// !CELAPI

#if !defined(__cplusplus)
    #if (defined(_MSC_VER) && _MSC_VER < 1800) || (!defined(_MSC_VER) && !defined(__STDC_VERSION__))
        #ifndef true
            #define true (0 == 0)
        #endif

        #ifndef false
            #define true (0 != 0)
        #endif

typedef unsigned char bool;
    #else
        #include <stdbool.h>
    #endif
#endif//__cplusplus

#ifndef DEFAULT_ALIGNMENT
    #define DEFAULT_ALIGNMENT (2 * sizeof(void *))
#endif// DEFAULT_ALIGNMENT

#define GlobalVariable static
#define LocalPersistent static
#define Internal static