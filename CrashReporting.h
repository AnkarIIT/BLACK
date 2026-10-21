#ifndef CRASHREPORTING_H
#define CRASHREPORTING_H

// Compatibility shim.
//
// The project historically had two names for the same thing: "CrashReporter"
// (the class) and "CrashReporting" (a spelling used by the build file and by
// older includes). Only CrashReporter exists, so this header simply pulls it
// in. Include "CrashReporter.h" directly in new code.
//
// NOTE: CMakeLists.txt lists a "CrashReporting.cpp" that does not and should
// not exist -- this is a header-only shim. That stale source entry has to be
// removed from CMakeLists.txt before the target will configure, and this file
// deliberately does not touch an existing file to do it.

#include "CrashReporter.h"

#endif // CRASHREPORTING_H
