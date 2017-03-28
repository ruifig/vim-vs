if !has('python')
	echoerr "vimvs requires Python support"
	finish
endif

if !exists('g:vimvs_exe')
	let g:vimvs_exe = resolve(fnamemodify(resolve(expand('<sfile>:p')), ':h') . "\\..\\bin\\vimvs.exe")
endif

if !exists('g:vimvs_Configuration')
	let g:vimvs_Configuration = "Debug"
endif

if !exists('g:vimvs_Platform')
	let g:vimvs_Platform = "x64"
endif

let g:vimvs_root = resolve(expand('<sfile>:p:h') . "\\..\\")

" TODO: Why do I have the 1 in [1, 'g:vimvs_exe'] ? Seems like garbage.
" Add "g:vimvs_exe" to g:ycm:extra_conf_vim_data
if exists('g:ycm_extra_conf_vim_data')
	if index(g:ycm_extra_conf_vim_data, 'g:vimvs_exe')==-1
		let g:ycm_extra_conf_vim_data = g:ycm_extra_conf_vim_data + [1, 'g:vimvs_exe']
	endif
else
	let g:ycm_extra_conf_vim_data = [1, 'g:vimvs_exe']
endif

" Load vimvs Python module
python << EOF
# Add python sources folder to the system path.
sys.path.insert( 0, vim.eval('g:vimvs_root') + 'plugin' )
import vimvs
reload(vimvs)
EOF

function! GetConfiguration()
	if exists('g:vimvs_Configuration')
		return g:vimvs_Configuration
	else
		return ""
endfunction

function! GetPlatform()
	if exists('g:vimvs_Platform')
		return g:vimvs_Platform
	else
		return ""
endfunction

function! LoadQuickfix()
python << EOF
vimvs.LoadQuickfix()
EOF
endfunction

function! CompileFile(file)
python << EOF
file = vim.eval("a:file")
vimvs.CompileFile(file)
EOF
endfunction

command! VimvsCompileFile call CompileFile(expand("%:p"))

function! Test()
" Start Python code 
python << EOF

import vim, os, subprocess, re

vimvs = vim.eval("g:vimvs_exe")

startupinfo = subprocess.STARTUPINFO()
startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
p = subprocess.Popen([vimvs, "-build=file:c:\\work\\crazygaze\\vim-vs\\source\\inifile.cpp"], stdout=subprocess.PIPE, startupinfo=startupinfo)
out, err = p.communicate()
print out
print p.returncode

# Get line starting with YCM
strings = re.search("^\s*1>(.*)", out, flags=re.MULTILINE)
if strings is None:
	print "No match found"
else:
	print "Found:" + strings.group(1)

EOF
" Ending Python code
endfunction
