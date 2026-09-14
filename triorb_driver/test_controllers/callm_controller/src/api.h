#pragma once

#if defined _WIN32 || defined __CYGWIN__
#  define CallMController_DLLIMPORT __declspec(dllimport)
#  define CallMController_DLLEXPORT __declspec(dllexport)
#  define CallMController_DLLLOCAL
#else
// On Linux, for GCC >= 4, tag symbols using GCC extension.
#  if __GNUC__ >= 4
#    define CallMController_DLLIMPORT __attribute__((visibility("default")))
#    define CallMController_DLLEXPORT __attribute__((visibility("default")))
#    define CallMController_DLLLOCAL __attribute__((visibility("hidden")))
#  else
// Otherwise (GCC < 4 or another compiler is used), export everything.
#    define CallMController_DLLIMPORT
#    define CallMController_DLLEXPORT
#    define CallMController_DLLLOCAL
#  endif // __GNUC__ >= 4
#endif   // defined _WIN32 || defined __CYGWIN__

#ifdef CallMController_STATIC
// If one is using the library statically, get rid of
// extra information.
#  define CallMController_DLLAPI
#  define CallMController_LOCAL
#else
// Depending on whether one is building or using the
// library define DLLAPI to import or export.
#  ifdef CallMController_EXPORTS
#    define CallMController_DLLAPI CallMController_DLLEXPORT
#  else
#    define CallMController_DLLAPI CallMController_DLLIMPORT
#  endif // CallMController_EXPORTS
#  define CallMController_LOCAL CallMController_DLLLOCAL
#endif // CallMController_STATIC
