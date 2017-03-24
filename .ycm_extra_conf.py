import os
import ycm_core
import logging
import subprocess
import re

# These are the compilation flags that will be used in case there's no
# compilation database set (by default, one is not set).
# CHANGE THIS LIST OF FLAGS. YES, THIS IS THE DROID YOU HAVE BEEN LOOKING FOR.
flags = [
'-Wall',
'-Wextra',
#'-Werror', # All warnings as errors
'-fexceptions',
'-DNDEBUG',
'-DCINTERFACE', # To let Clang parge VS's combaseapi.h, otherwise we get an error "unknown type name 'IUnknown' "

# mscv compatibility
'-fms-compatibility',
'-fms-compatibility-version=19',
'-fdelayed-template-parsing',

# According to this? : https://github.com/Valloric/YouCompleteMe/issues/1932
# '--target', '<arch>-pc-windows-msvc<xx.yy.zzzzz>'

# For a C project, you would set this to something like 'c99' instead of
# 'c++11'.
'-std=c++14',

# ...and the same thing goes for the magic -x option which specifies the
# language that the files to be compiled are written in. This is mostly
# relevant for c++ headers.
# For a C project, you would set this to 'c' instead of 'c++'.
'-x', 'c++',

# System includes
'-isystem', 'C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include',
'-isystem', 'C:/Program Files (x86)/Windows Kits/10/Include/10.0.10150.0/ucrt',
'-isystem', 'C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/atlmfc/include',
'-isystem', 'C:/Program Files (x86)/Windows Kits/NETFXSDK/4.6/include/um',
'-isystem', 'C:/Program Files (x86)/Windows Kits/8.1/Include/um',
'-isystem', 'C:/Program Files (x86)/Windows Kits/8.1/Include/shared',
'-isystem', 'C:/Program Files (x86)/Windows Kits/8.1/Include/winrt',
]

# Set this to the absolute path to the folder (NOT the file!) containing the
# compile_commands.json file to use that instead of 'flags'. See here for
# more details: http://clang.llvm.org/docs/JSONCompilationDatabase.html
#
# You can get CMake to generate this file for you by adding:
#   set( CMAKE_EXPORT_COMPILE_COMMANDS 1 )
# to your CMakeLists.txt file.
#
# Most projects will NOT need to set this to anything; you can just change the
# 'flags' list of compilation flags. Notice that YCM itself uses that approach.
#compilation_database_folder = 'c:/work/crazygaze/vim-vs'

# Get the path of the this script, according to: http://stackoverflow.com/questions/4934806/how-can-i-find-scripts-directory-with-python
compilation_database_folder = os.path.dirname(os.path.realpath(__file__))

if os.path.exists( compilation_database_folder ):
	database = ycm_core.CompilationDatabase( compilation_database_folder )
else:
	database = None

SOURCE_EXTENSIONS = [ '.cpp', '.cxx', '.cc', '.c', '.m', '.mm' ]

def DirectoryOfThisScript():
	return os.path.dirname( os.path.abspath( __file__ ) )

def MakeRelativePathsInFlagsAbsolute( flags, working_directory ):
	if not working_directory:
		return list( flags )
	new_flags = []
	make_next_absolute = False
	path_flags = [ '-isystem', '-I', '-iquote', '--sysroot=' ]

	for flag in flags:
		new_flag = flag
		
		if make_next_absolute:
			make_next_absolute = False
			if not flag.startswith( '/' ):
				new_flag = os.path.join( working_directory, flag )
		
		for path_flag in path_flags:
			if flag == path_flag:
				make_next_absolute = True
				break
			
			if flag.startswith( path_flag ):
				path = flag[ len( path_flag ): ]
				new_flag = path_flag + os.path.join( working_directory, path )
				break
		
		if new_flag:
		  new_flags.append( new_flag )

	return new_flags


def IsHeaderFile( filename ):
	extension = os.path.splitext( filename )[ 1 ]
	return extension in [ '.h', '.hxx', '.hpp', '.hh' ]


def GetCompilationInfoForFile( filename ):
	return database.GetCompilationInfoForFile( filename )

vimvs_exe = ""

def Vimvs_getycm( filename ):
	global vimvs_exe
	startupinfo = subprocess.STARTUPINFO()
	startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
	p = subprocess.Popen([vimvs_exe, '-getycm=' + filename], stdout=subprocess.PIPE, startupinfo=startupinfo)
	out,err = p.communicate()
	if p.returncode>0:
		raise Exception("VIMVS: getycm failed")
	strings = re.search("^\s*YCM_CMD:(.*)", out, flags=re.MULTILINE)
	if strings is None:
		raise Exception("VIMVS: error parsing getycm output. Could not find YCM_CMD line.")

	res = []
	cmds = strings.group(1).split("|")
	for cmd in cmds:
		if cmd.strip()!="":
			res.append(cmd)
	return res

def FlagsForFile( filename, **kwargs ):
	global vimvs_exe
	if not (kwargs and 'client_data' in kwargs):
		raise Exception("VIMVS: client_data not found in kwargs")
	client_data = kwargs['client_data']
	if not (client_data and 'g:vimvs_exe' in client_data):
		raise Exception("VIMVS: g:vimvs_exe not present in client_data")
	vimvs_exe = client_data['g:vimvs_exe']
	#raise Exception(vimvs_exe)
	cmd = Vimvs_getycm( filename )
	return {
		'flags' : cmd,
		'do_cache' : True
	}

'''

def FlagsForFile( filename, **kwargs ):
	if database:
		# Bear in mind that compilation_info.compiler_flags_ does NOT return a
		# python list, but a "list-like" StringVec object
		compilation_info = GetCompilationInfoForFile( filename )
		if not compilation_info:
		  return None
		
		final_flags = MakeRelativePathsInFlagsAbsolute(
		  compilation_info.compiler_flags_,
		  compilation_info.compiler_working_dir_ )
	else:
		relative_to = DirectoryOfThisScript()
		final_flags = MakeRelativePathsInFlagsAbsolute( flags, relative_to )
	
	return {
		'flags': final_flags,
		'do_cache': True
	}

'''

