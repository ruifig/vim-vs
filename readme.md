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

* VIM >=8.0
* AsyncRun plugin [](https://github.com/skywind3000/asyncrun.vim)
	* This is required so that vim-vs doesn't block VIM while building
* Visual Studio >=2013

Use
---

FILL ME

