#pragma once

#ifdef _WIN32
#include "targetver.h"
#endif

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <shellapi.h>
#endif


// If set to 1, and running on Debug and Windows, it will enable some more CRT memory debug things
#define ENABLE_MEM_DEBUG 0

#include "crazygaze/vimvs/vimvs.h"

#include <stdio.h>
#include <vector>
#include <string>
#include <queue>
#include <regex>
#include <fstream>
#include <assert.h>

#include "Semaphore.h"

//
// UnitTest++
//
#include "UnitTest++/UnitTest++.h"
#include "UnitTest++/CurrentTest.h"

