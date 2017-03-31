#pragma once

//
// Disable some code analysis warnings
//

// warning C6326: Potential comparison of a constant with another constant.
#pragma warning( disable: 6326 )

#ifdef _WIN32
#include "targetver.h"
#endif

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <shellapi.h>
	#include <Shlwapi.h>
	#include <Psapi.h>
#endif

// If set to 1, and running on Debug and Windows, it will enable some more CRT memory debug things
#define ENABLE_MEM_DEBUG 0

#include <stdio.h>
#include <set>
#include <vector>
#include <string>
#include <queue>
#include <regex>
#include <thread>
#include <fstream>
#include <assert.h>
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <future>
#include <memory>
#include <Strsafe.h>

#pragma warning( push )
// Disable : "decorated name length exceeded, name was truncated"
// https://msdn.microsoft.com/en-us/library/074af4b6.aspx
#pragma warning(disable:4503)
// Disable:	'=': conversion from 'std::_Array_iterator<_Ty,64>::difference_type' to 'long', possible loss of data
#pragma warning(disable:4244)
#include "3rdparty/json.hpp"
#pragma warning( pop )

#include "3rdparty/sqlite/sqlite3.h"

#include "3rdparty/MurmurHash/MurmurHash3.h"
