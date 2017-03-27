
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

function! LoadQuickfix()
" Start Python code
python << EOF

import vim,os

with open("C:/work/crazygaze/vim-vs/.vimvs.quickfix") as f:
	lines = f.readlines()
qf = []
print lines
# Format of the file is: File|Line|Col|Type|Code|Message
for l in lines:
	tokens = l.split("|")
	print tokens[0]
	qf.append(dict(
			filename=tokens[0].replace("\\", "/"),
			lnum=tokens[1], 
			text=tokens[5]))
 
vim.eval('setqflist(%s)' % qf)
#print qf
#print qf[0].get('filename')

EOF
" Ending Python code

endfunction

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
