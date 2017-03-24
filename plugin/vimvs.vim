
if !exists('g:vimvs_exe')
	"let g:vimvs_exe = resolve(fnamemodify(resolve(expand('<sfile>:p')), ':h') . "\\..\\bin\\vimvs.exe")
	let g:vimvs_exe = resolve(fnamemodify(resolve(expand('<sfile>:p')), ':h') . "\\..\\build\\bin\\vimvs_x64_Debug.exe")
endif

" Add "g:vimvs_exe" to g:ycm:extra_conf_vim_data
if exists('g:ycm_extra_conf_vim_data')
	if index(g:ycm_extra_conf_vim_data, 'g:vimvs_exe')==-1
		let g:ycm_extra_conf_vim_data = g:ycm_extra_conf_vim_data + [1, 'g:vimvs_exe']
	endif
else
	let g:ycm_extra_conf_vim_data = [1, 'g:vimvs_exe']
endif

" let g:ycm_global_ycm_extra_conf = 'B:\Link\Work\crazygaze\vim-vs\.ycm_extra_conf.py'
" let g:ycm_global_ycm_extra_conf = 'C:\.ycm_extra_conf.py'
" let g:ycm_global_ycm_extra_conf = 'B:\Dropbox\utils\vim\vimfiles\bundle\vim-vs\.ycm_extra_conf.py'
" let g:ycm_global_ycm_extra_conf = '~/.ycm_extra_conf.py'
" let g:ycm_global_ycm_extra_conf = expand('~/.ycm_extra_conf.py')

"let g:ycm_global_ycm_extra_conf = 'C:\.ycm_extra_conf.py'

function! Test()
" Start Python code
python << EOF

import vim, os, subprocess, re

vimvs = vim.eval("g:vimvs_exe")

vim.current.uffer.append(vimvs)
# out = subprocess.check_output([vimvs, "-help"], shell=False)

startupinfo = subprocess.STARTUPINFO()
startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
p = subprocess.Popen([vimvs, "-help"], stdout=subprocess.PIPE, startupinfo=startupinfo)
out, err = p.communicate()

print out
print p.returncode

p = subprocess.Popen([vimvs, "-getycm=xxx"], stdout=subprocess.PIPE, startupinfo=startupinfo)
out, err = p.communicate()
print out
print p.returncode

# Get line starting with YCM
strings = re.search("^\s*YCM_CMD:(.*)", out, flags=re.MULTILINE)
if strings is None:
	print "No match found"
else:
	print "Found:" + strings.group(1)



EOF
" Ending Python code
endfunction

function! Test2()
" Start Python code
python << EOF

import vim, os, subprocess, re

s = "AA|BB||C C|DD| |\r"
strings = s.split("|")
print strings
cmds = []
for s in strings:
	if s.strip()!="":
		cmds.append(s)
print cmds

EOF
" Ending Python code
endfunction

