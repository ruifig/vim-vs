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

function! GetConfigurationAndPlatform()
	let configuration = GetConfiguration()
	let platform = GetPlatform()
	let res = ""
	if configuration != ""
		let res = ' -configuration=' . configuration
	endif
	if platform != ""
		let res = res . ' -platform=' . platform
	endif
	return res
endfunction

function! LoadQuickfix()
python << EOF
vimvs.LoadQuickfix()
EOF
endfunction

function! CompileFile2(file)
python << EOF
file = vim.eval("a:file")
vimvs.CompileFile(file)
EOF
endfunction

function! vimvs#GetRoot()
python << EOF
# Notes:
#	repr so it convert any character to a way that I can pass them to the vim.command
#	[1:-1] so it removes the single quotes at the star and end that repr puts in there
vim.command("let res = \"%s\"" % repr(vimvs.GetRoot())[1:-1])
EOF
return res
endfunction

function! Build()
	let cmd = g:vimvs_exe . ' -build' . GetConfigurationAndPlatform()
	execute 'AsyncRun -post=:call\ LoadQuickfix() @' . cmd
endfunction

function! vimvs#CompileFile(file)
	let cmd = g:vimvs_exe . ' -build=file:"' . a:file . '"' . GetConfigurationAndPlatform()
	echo cmd
	execute 'AsyncRun -post=:call\ LoadQuickfix() @' . cmd
endfunction

command! VimvsCompileFile call vimvs#CompileFile(expand("%:p"))

