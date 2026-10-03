#pragma once

// Platform detection for mobile platforms
// godot-cpp defines ANDROID_ENABLED and IOS_ENABLED in cmake/android.cmake and cmake/ios.cmake

#if defined(ANDROID_ENABLED) || defined(IOS_ENABLED)
#define PLATEAU_MOBILE_PLATFORM
#endif

// iOS builds the CityGML parser (Xerces-C and GLU's tessellator, see thirdparty/glu and
// patches/), so CityGML loading and mesh extraction work there. Android still uses
// libcitygml's dummy parser.
#if defined(PLATEAU_MOBILE_PLATFORM) && !defined(IOS_ENABLED)
#define PLATEAU_NO_CITYGML_PARSER
#endif

// Error macros for unsupported features on mobile platforms
// Following Unity SDK approach: return error instead of crashing

#ifdef PLATEAU_MOBILE_PLATFORM
#define PLATEAU_MOBILE_UNSUPPORTED_V(ret) \
    ERR_FAIL_V_MSG(ret, "This feature is not supported on mobile platforms. Use pre-converted assets.")
#define PLATEAU_MOBILE_UNSUPPORTED() \
    ERR_FAIL_MSG("This feature is not supported on mobile platforms. Use pre-converted assets.")
#else
#define PLATEAU_MOBILE_UNSUPPORTED_V(ret) ((void)0)
#define PLATEAU_MOBILE_UNSUPPORTED() ((void)0)
#endif
