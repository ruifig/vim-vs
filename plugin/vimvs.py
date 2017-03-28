import os
import vim
import subprocess
import re

vimvs_exe = vim.eval("g:vimvs_exe")
vimvs_root = vim.eval("g:vimvs_root")

def LoadQuickfix():
	with open("C:/work/crazygaze/vim-vs/.vimvs.quickfix") as f:
		lines = f.readlines()
	qf = []
	#print lines
	# Format of the file is: File|Line|Col|Type|Code|Message
	for l in lines:
		tokens = l.split("|")
		print tokens[0]
		qf.append(dict(
				filename=tokens[0].replace("\\", "/"),
				lnum=tokens[1], 
				text=tokens[5]))
	vim.eval('setqflist(%s)' % qf)

def SetConfigurationAndPlatform(lst):
	configuration = vim.eval("GetConfiguration()")
	platform = vim.eval("GetPlatform()")
	if configuration != "":
		lst.append("-configuration=" + configuration)
	if platform != "":
		lst.append("-platform=" + platform)
	return lst

def CompileFile(file):
	startupinfo = subprocess.STARTUPINFO()
	startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
	args = [vimvs_exe, "-build=file:" + file]
	args = SetConfigurationAndPlatform(args)
	p = subprocess.Popen(args, stdout=subprocess.PIPE, startupinfo=startupinfo)
	out, err = p.communicate()
	print out
	print p.returncode
	# Get line starting with YCM
	strings = re.search("^\s*1>(.*)", out, flags=re.MULTILINE)
	if strings is None:
		print "No match found"
	else:
		print "Found:" + strings.group(1)

