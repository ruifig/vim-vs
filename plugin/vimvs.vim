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

function! vimvs#LoadQuickfix()
python << EOF
vimvs.LoadQuickfix()
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

function! vimvs#HasRoot()
python << EOF
# Notes:
#	repr so it convert any character to a way that I can pass them to the vim.command
#	[1:-1] so it removes the single quotes at the star and end that repr puts in there
vim.command("let res = '%s'" % vimvs.HasRoot())
EOF
if res != 'True'
	echoerr "No vimvs root detected"
	return 0
else
	return 1
endif
endfunction

function! vimvs#Build()
	if !vimvs#HasRoot()
		return
	endif
	let cmd = g:vimvs_exe . ' -build' . vimvs#GetConfigurationAndPlatformCmd()
	execute 'AsyncRun -post=:call\ vimvs\#LoadQuickfix() @' . cmd
endfunction

function! vimvs#Rebuild()
	if !vimvs#HasRoot()
		return
	endif
	let cmd = g:vimvs_exe . ' -build=prj:Rebuild' . vimvs#GetConfigurationAndPlatformCmd()
	execute 'AsyncRun -post=:call\ vimvs\#LoadQuickfix() @' . cmd
endfunction

function! vimvs#BuildDB()
	if !vimvs#HasRoot()
		return
	endif
	let cmd = g:vimvs_exe . ' -builddb=prj:Rebuild' . vimvs#GetConfigurationAndPlatformCmd()
	execute 'AsyncRun -post=:call\ vimvs\#LoadQuickfix() @' . cmd
endfunction

function! vimvs#Clean()
	if !vimvs#HasRoot()
		return
	endif
	let cmd = g:vimvs_exe . ' -build=prj:Clean' . vimvs#GetConfigurationAndPlatformCmd()
	execute 'AsyncRun -post=:call\ vimvs\#LoadQuickfix() @' . cmd
endfunction

function! vimvs#CompileFile(file)
	if !vimvs#HasRoot()
		return
	endif
	let cmd = g:vimvs_exe . ' -build=file:"' . a:file . '"' . vimvs#GetConfigurationAndPlatformCmd()
	echo cmd
	execute 'AsyncRun -post=:call\ vimvs\#LoadQuickfix() @' . cmd
endfunction

function! vimvs#GetAlt(file)
python << EOF
vim.command("let res = \"%s\"" % repr(vimvs.GetAlt(vim.eval('a:file')))[1:-1])
EOF
return res
endfunction

function! vimvs#OpenAlt(file)
	let res = vimvs#GetAlt(a:file)
	if !empty(res)
		execute "edit " res
	endif
endfunction

command! VimvsBuild call vimvs#Build()
command! VimvsRebuild call vimvs#Rebuild()
command! VimvsBuildDB call vimvs#BuildDB()
command! VimvsClean call vimvs#Clean()
command! VimvsCompile call vimvs#CompileFile(expand("%:p"))
command! VimvsGetAlt call vimvs#GetAlt(expand("%:p"))
command! VimvsOpenAlt call vimvs#OpenAlt(expand("%:p"))

