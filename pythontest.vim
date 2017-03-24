
" From https://dzone.com/articles/how-write-vim-plugins-python
function! PythonTest()

" Get folder of this script
let g:vimvs_path =        fnamemodify(resolve(expand('<sfile>:p')), ':h')
let g:vimvs_exe = resolve(fnamemodify(resolve(expand('<sfile>:p')), ':h') . "\\bin\\vimvs.exe")
"put =s:vimvs_path


" Start Python code
python << EOF

import vim, os

vim.current.buffer.append("1: " + vim.eval("g:vimvs_path") )
vim.current.buffer.append("2: " + vim.eval("g:vimvs_exe") )
vim.current.buffer.append("Testing...\n")
#vim.current.buffer.append(os.path.dirname( os.path.abspath( __file__ )))

EOF
" Ending Python code

endfunction
