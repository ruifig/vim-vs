if !has('python')
	echoerr "vimvs requires Python support"
	finish
endif

let g:vimvs_plugin_root = resolve(expand('<sfile>:p:h') . "\\..\\")

if !exists('g:vimvs_exe')
	let g:vimvs_exe = resolve(fnamemodify(resolve(expand('<sfile>:p')), ':h') . "\\..\\bin\\vimvs.exe")
endif

if !exists('g:vimvs_configuration')
	let g:vimvs_configuration = "Debug"
endif

if !exists('g:vimvs_platform')
	let g:vimvs_platform = "x64"
endif

" If 1, it will save all files before running any build/compile commands
if !exists('g:vimvs_wa')
	let g:vimvs_wa = 0
endif

if !exists('g:ycm_global_ycm_extra_conf')
	let g:ycm_global_ycm_extra_conf = g:vimvs_plugin_root + 'plugin/.ycm_extran_conf.py'
endif

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
import vim
sys.path.insert( 0, vim.eval('g:vimvs_plugin_root') + 'plugin' )
import vimvs
reload(vimvs)
EOF

"
" ========================== Utility function
"
function! vimvs#PrintError(msg) abort
    execute 'normal! \<Esc>'
    echohl WarningMsg
    echomsg a:msg
    echohl None
endfunction

function! vimvs#GetConfiguration()
	if exists('g:vimvs_configuration')
		return g:vimvs_configuration
	else
		return ""
endfunction

function! vimvs#GetPlatform()
	if exists('g:vimvs_platform')
		return g:vimvs_platform
	else
		return ""
endfunction

function! vimvs#GetConfigurationAndPlatformCmd()
	let configuration = vimvs#GetConfiguration()
	let platform = vimvs#GetPlatform()
	let res = ""
	if configuration != ""
		let res = ' -configuration=' . configuration
	endif
	if platform != ""
		let res = res . ' -platform=' . platform
	endif
	return res
endfunction

"
" ========================== User functions
"

function! vimvs#GetRoot()
	let res = ""
python << EOF
# Notes:
#	repr so it convert any character to a way that I can pass them to the vim.command
#	[1:-1] so it removes the single quotes at the start and end that repr puts in there
try:
	vim.command("let res = %s" % vimvs.ToVimString(vimvs.GetRoot()))
except RuntimeError as e:
	vimvs.PrintError(e.message)
EOF
	return res
endfunction

"
"
function! vimvs#LoadQuickfix()
	if empty(vimvs#GetRoot())
		return
	endif
python << EOF
try:
	vimvs.LoadQuickfix()
except RuntimeError as e:
	vimvs.PrintError(e.message)
EOF
endfunction

"
"
function! vimvs#Build()
	if empty(vimvs#GetRoot())
		return
	endif
	if g:vimvs_wa
		wa
	endif
	let cmd = g:vimvs_exe . ' -build' . vimvs#GetConfigurationAndPlatformCmd()
	execute 'AsyncRun -post=:call\ vimvs\#LoadQuickfix() @' . cmd
endfunction

"
"
function! vimvs#Rebuild()
	if empty(vimvs#GetRoot())
		return
	endif
	if g:vimvs_wa
		wa
	endif
	let cmd = g:vimvs_exe . ' -build=prj:Rebuild' . vimvs#GetConfigurationAndPlatformCmd()
	execute 'AsyncRun -post=:call\ vimvs\#LoadQuickfix() @' . cmd
endfunction

"
"
function! vimvs#BuildDB(fastparser)
	if empty(vimvs#GetRoot())
		return
	endif
	if g:vimvs_wa
		wa
	endif
	if a:fastparser
		let cmd = g:vimvs_exe . ' -fastparser -builddb' . vimvs#GetConfigurationAndPlatformCmd()
	else
		let cmd = g:vimvs_exe . ' -builddb=prj:Rebuild' . vimvs#GetConfigurationAndPlatformCmd()
	endif
	execute 'AsyncRun -post=:call\ vimvs\#LoadQuickfix() @' . cmd
endfunction

"
"
function! vimvs#Clean()
	if empty(vimvs#GetRoot())
		return
	endif
	if g:vimvs_wa
		wa
	endif
	let cmd = g:vimvs_exe . ' -build=prj:Clean' . vimvs#GetConfigurationAndPlatformCmd()
	<execute 'AsyncRun -post=:call\ vimvs\#LoadQuickfix() @' . cmd
endfunction

"
"
function! vimvs#CompileFile(file)
	if empty(vimvs#GetRoot())
		return
	endif
	if g:vimvs_wa
		wa
	endif
	let cmd = g:vimvs_exe . ' -build=file:"' . a:file . '"' . vimvs#GetConfigurationAndPlatformCmd()
	echo cmd
	execute 'AsyncRun -post=:call\ vimvs\#LoadQuickfix() @' . cmd
endfunction

"
"
function! vimvs#GetAlt(file)
	let res = ""
	if empty(vimvs#GetRoot())
		return ""
	endif
python << EOF
try:
	vim.command("let res = %s" % vimvs.ToVimString(vimvs.GetAlt(vim.eval('a:file'))))
except RuntimeError as e:
	vimvs.PrintError(e.message)
EOF
	return res
endfunction

"
"
function! vimvs#OpenAlt(file)
	let res = vimvs#GetAlt(a:file)
	if !empty(res)
		execute "edit " res
	endif
endfunction

command! VimvsRoot echo vimvs#GetRoot()
command! VimvsActiveConfig echo vimvs#GetConfiguration() "|" vimvs#GetPlatform()
command! -nargs=1 VimvsSetConfiguration execute("let g:vimvs_configuration='" . <f-args> . "'")
command! -nargs=1 VimvsSetPlatform execute("let g:vimvs_platform='" . <f-args> . "'")
command! VimvsBuild call vimvs#Build()
command! VimvsRebuild call vimvs#Rebuild()
command! VimvsBuildDB call vimvs#BuildDB(0)
command! VimvsBuildDBFast call vimvs#BuildDB(1)
command! VimvsClean call vimvs#Clean()
command! VimvsCompile call vimvs#CompileFile(expand("%:p"))
command! VimvsGetAlt call vimvs#GetAlt(expand("%:p"))
command! VimvsOpenAlt call vimvs#OpenAlt(expand("%:p"))
