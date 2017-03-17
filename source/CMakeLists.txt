cmake_minimum_required(VERSION 3.5)
project(vimvs)

ucm_add_files(
	"3rdparty/json.hpp"
	"ChildProcessLauncher.cpp"
	"ChildProcessLauncher.h"
	"Database.h"
	"IniFile.cpp"
	"IniFile.h"
	"Logging.cpp"
	"Logging.h"
	"Parameters.cpp"
	"Parameters.h"
	"Parser.cpp"
	"Parser.h"
	"ScopeGuard.h"
	"SqliteWrapper.h"
	"SqliteWrapper.cpp"
	"targetver.h"
	"Utils.cpp"
	"Utils.h"
	"vimvs.cpp"
	 TO VIMVS_SRC
	 )

ADD_MSVC_PRECOMPILED_HEADER("vimvsPCH.h" "vimvsPCH.cpp" VIMVS_SRC)

ucm_add_files(
	"3rdparty/sqlite/sqlite3.h"
	"3rdparty/sqlite/sqlite3.c"
	TO VIMVS_SRC
	)

# Disable some code analysis warnings in sqlite
set_property(SOURCE "3rdparty/sqlite/sqlite3.c" APPEND PROPERTY COMPILE_FLAGS "/wd28251 /wd28182 /wd6326 /wd6011 /wd6239 /wd6001 /wd6385 /wd6387 /wd6240 /wd6386 /wd6313")
#set_source_files_properties("3rdparty/sqlite/sqlite3.c" PROPERTIES COMPILE_FLAGS "/wd28251 /wd28182 /wd6326 /wd6011 /wd6239 /wd6001 /wd6385 /wd6387 /wd6240 /wd6386 /wd6313")

add_executable(vimvs ${VIMVS_SRC})
cz_set_postfix()
cz_add_common_libs()
target_compile_definitions(vimvs PRIVATE NOMINMAX)

# By defining these macros, the Visual Studio generator picks it up, and sets the project to use Unicode
target_compile_definitions(vimvs PRIVATE UNICODE _UNICODE)

# Uncomment this to enable code analysis
target_compile_options(vimvs PRIVATE)

target_link_libraries(vimvs Psapi.lib)

cz_set_postfix()

