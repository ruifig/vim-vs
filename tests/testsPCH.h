#pragma once

#ifdef _WIN32
#include "targetver.h"
#endif

// If set to 1, and running on Debug and Windows, it will enable some more CRT memory debug things
#define ENABLE_MEM_DEBUG 0

#include "crazygaze/vimvs/vimvs.h"

#include <stdio.h>
#include <vector>
#include <string>
#include <queue>

#include "Semaphore.h"

//
// UnitTest++
//
#include "UnitTest++/UnitTest++.h"
#include "UnitTest++/CurrentTest.h"

