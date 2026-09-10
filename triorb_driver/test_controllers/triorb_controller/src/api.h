#pragma once

#if defined _WIN32 || defined __CYGWIN__
#  define TriOrbController_DLLIMPORT __declspec(dllimport)
#  define TriOrbController_DLLEXPORT __declspec(dllexport)
#  define TriOrbController_DLLLOCAL
#else
// On Linux, for GCC >= 4, tag symbols using GCC extension.
#  if __GNUC__ >= 4
#    define TriOrbController_DLLIMPORT __attribute__((visibility("default")))
#    define TriOrbController_DLLEXPORT __attribute__((visibility("default")))
#    define TriOrbController_DLLLOCAL __attribute__((visibility("hidden")))
#  else
// Otherwise (GCC < 4 or another compiler is used), export everything.
#    define TriOrbController_DLLIMPORT
#    define TriOrbController_DLLEXPORT
#    define TriOrbController_DLLLOCAL
#  endif // __GNUC__ >= 4
#endif   // defined _WIN32 || defined __CYGWIN__

#ifdef TriOrbController_STATIC
// If one is using the library statically, get rid of
// extra information.
#  define TriOrbController_DLLAPI
#  define TriOrbController_LOCAL
#else
// Depending on whether one is building or using the
// library define DLLAPI to import or export.
#  ifdef TriOrbController_EXPORTS
#    define TriOrbController_DLLAPI TriOrbController_DLLEXPORT
#  else
#    define TriOrbController_DLLAPI TriOrbController_DLLIMPORT
#  endif // TriOrbController_EXPORTS
#  define TriOrbController_LOCAL TriOrbController_DLLLOCAL
#endif // TriOrbController_STATIC
