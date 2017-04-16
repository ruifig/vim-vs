vim-vs
------

Vim-vs is a vim plugin to make it easier to build Visual Studio's C/C++ solutions.

Vim-vs only targets Windows and Visual Studio, since the target audience is programmers that needs to use Windows and Visual Studio but want use Vim for editing.

Supported features:

* Build/Rebuild/Clean the entire solution 
* Compile a single file
* Provides compile parameters to [YouCompleteMe](https://github.com/Valloric/YouCompleteMe), making it extremely easy to setup YouCompleteMe for a project
* Support for Visual Studio 2013/2015/2017 solutions/projects
	* Might work with earlier Visual Studio solutions
	* No support for custom build commands (As-in: A file is compiled with something else other than CL.exe)
* Build is done asynchronously to the quickfix window

Requirements to use:

* VIM >=8.0 with Python 2
* AsyncRun plugin [https://github.com/skywind3000/asyncrun.vim]
	* This is required so that vim-vs doesn't block VIM while building
* Visual Studio >=2013

Installation
------------

FILL ME (install AsyncRun if required, add whatever is necessary to vimrc)
	
How to use
-----------

Create a ```.vimvs.ini``` file at the root of your project with the following contents:

```
[General]
solution=<Path to your sln file>
common_ycm_params=-std=c++14|-Wall|-Wextra|-fexceptions|-Wno-microsoft|
```

Tweak **common_ycm_params** as required for your project if necessary. Individual parameters should be seperated by ```|```
Take a look at vim-vs's own ```.vimvs.ini``` for a working example.

**Commands**

* ```:VimvsRoot```
	* Display the directory where ```.vimvs.ini``` is found
* ```:VimvsUpdateDB```
	* This will perform a fake build using to build the database vim-vs requires for any other commands.
	* You need to run this when you feel vim-vs is not up to date, such as when you add/remove files, or add/remove #include statements. For example, if you add a new source file to a project, you won't be able to compile that file (aka: Ctr-F7 in Visual Studio) until you run this command.
* ```:VimvsActiveConfig```
	* Displays what Configuration|Platform is active.
* ```:VimvsSetConfiguration <Configuration>```
	* Set the configuration to use. E.g: ```:VimvsSetConfiguration Release```
	* Default is ```Debug```
* ```:VimvsSetPlatform <Platform>```
	* Set the platform to use. E.g: ```:VimvsSetPlatform x86```
	* Default is ```x64```
* ```:VimvsBuild```
	* Perform a build
* ```:VimvsRebuild```
	* Perform a rebuild
* ```:VimvsClean```
	* Perform a rebuild
* ```:VimvsCompile```
	* Compile only the current file
* ```:VimvsOpenAlt```
	* Swaps between a header/source file

Commands that involve building anything will show current progress in the quickfix list. Once the command finishes, the quickfix list is reset to show any errors/warnings.

Behind the scenes
---

FILL ME

