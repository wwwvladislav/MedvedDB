#pragma once

#if defined(_AIX)
    #define MDV_PLATFORM_IBM_AIX                                                // IBM AIX
#elseif defined(__hpux)
    #define MDV_PLATFORM_HPUX                                                   // Hewlett-Packard HP-UX
#elif defined(_WIN32) || defined(_WIN64)
   #define MDV_PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR == 1
        #define MDV_PLATFORM_IOS_XCODE_SIMULATOR                                // iOS in Xcode simulator
    #elif TARGET_OS_IPHONE == 1
        #define MDV_PLATFORM_IOS                                                // iOS on iPhone, iPad, etc.
    #elif TARGET_OS_MAC == 1
	#define MDV_PLATFORM_OSX                                                // OSX
    #endif
#elif defined(__sun) && defined(__SVR4)
    #define MDV_PLATFORM_SOLARIS                                                // Solaris
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
   #define MDV_PLATFORM_BSD                                                     // FreeBSD, OpenBSD, NetBSD
#elif defined(__linux__)
   #define MDV_PLATFORM_LINUX
#endif

