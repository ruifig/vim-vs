cmake_minimum_required(VERSION 3.5)
project(vim-vs)

include(cmake/ucm.cmake)
include(cmake/utils.cmake)

set_property( GLOBAL PROPERTY USE_FOLDERS ON )

# Remove the MinSizeRel and RelWithDebInfo configs
# https://cmake.org/Wiki/CMake_FAQ#How_can_I_specify_my_own_configurations_.28for_generators_that_allow_it.29_.3F
if(CMAKE_CONFIGURATION_TYPES)

	#set_property( GLOBAL PROPERTY DEBUG_CONFIGURATIONS "Debug;DebugAnalyze" )
	#set(CMAKE_CONFIGURATION_TYPES Debug Release DebugAnalyze)

	set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
		"Reset the configurations to what we need"
 		FORCE)

	#set(CMAKE_CXX_FLAGS_DEBUGANALYZE ${CMAKE_CXX_FLAGS_DEBUG})
	#set(CMAKE_EXE_LINKER_FLAGS_DEBUGANALYZE ${CMAKE_EXE_LINKER_FLAGS_DEBUG})

endif()

# Debug info is generated for Release configurations.
if(MSVC)
	ucm_add_flags(/Zi CONFIG Release)
	if (MSVC12)
		ucm_add_linker_flags(EXE MODULE /DEBUG CONFIG Release)
	elseif(MSVC14)
		ucm_add_linker_flags(EXE MODULE /DEBUG:Fastlink CONFIG Release)
	endif()
endif()


# Will cause building to display what compiler parameters are being used, among other things
SET(CMAKE_VERBOSE_MAKEFILE_ON)

add_subdirectory(source)
add_subdirectory(UnitTest++)
add_subdirectory(tests)

