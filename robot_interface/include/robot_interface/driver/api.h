/*
 * Copyright 2015-2026 CNRS-UM LIRMM, CNRS-AIST JRL
 */

#pragma once

// Package version (header).
#define ROBOT_DRIVER_VERSION "UNKNOWN-dirty"

// Handle portable symbol export.
// Defining manually which symbol should be exported is required
// under Windows whether MinGW or MSVC is used.
//
// The headers then have to be able to work in two different modes:
// - dllexport when one is building the library,
// - dllimport for clients using the library.
//
// On Linux, set the visibility accordingly. If C++ symbol visibility
// is handled by the compiler, see: http://gcc.gnu.org/wiki/Visibility
#if defined _WIN32 || defined __CYGWIN__
// On Microsoft Windows, use dllimport and dllexport to tag symbols.
#  define ROBOT_DRIVER_DLLIMPORT __declspec(dllimport)
#  define ROBOT_DRIVER_DLLEXPORT __declspec(dllexport)
#  define ROBOT_DRIVER_DLLLOCAL
#else
// On Linux, for GCC >= 4, tag symbols using GCC extension.
#  if __GNUC__ >= 4
#    define ROBOT_DRIVER_DLLIMPORT __attribute__((visibility("default")))
#    define ROBOT_DRIVER_DLLEXPORT __attribute__((visibility("default")))
#    define ROBOT_DRIVER_DLLLOCAL __attribute__((visibility("hidden")))
#  else
// Otherwise (GCC < 4 or another compiler is used), export everything.
#    define ROBOT_DRIVER_DLLIMPORT
#    define ROBOT_DRIVER_DLLEXPORT
#    define ROBOT_DRIVER_DLLLOCAL
#  endif // __GNUC__ >= 4
#endif   // defined _WIN32 || defined __CYGWIN__

#ifdef ROBOT_DRIVER_STATIC
// If one is using the library statically, get rid of
// extra information.
#  define ROBOT_DRIVER_DLLAPI
#  define ROBOT_DRIVER_LOCAL
#else
// Depending on whether one is building or using the
// library define DLLAPI to import or export.
#  ifdef ROBOT_DRIVER_EXPORTS
#    define ROBOT_DRIVER_DLLAPI ROBOT_DRIVER_DLLEXPORT
#  else
#    define ROBOT_DRIVER_DLLAPI ROBOT_DRIVER_DLLIMPORT
#  endif // ROBOT_DRIVER_EXPORTS
#  define ROBOT_DRIVER_LOCAL ROBOT_DRIVER_DLLLOCAL
#endif // ROBOT_DRIVER_STATIC
