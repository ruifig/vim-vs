import os
import vim
import subprocess
import re

vimvs_exe = vim.eval("g:vimvs_exe")
vimvs_plugin_root = vim.eval("g:vimvs_plugin_root")

def GetRoot():
	startupinfo = subprocess.STARTUPINFO()
	startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
	p = subprocess.Popen([vimvs_exe, '-getroot'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, startupinfo=startupinfo)
	out,err = p.communicate()
	if p.returncode>0:
		raise Exception("VIMVS: -getroot failed")
	strings = re.search("^\s*ROOT:(.*)", out, flags=re.MULTILINE)
	if strings is None:
		raise Exception("VIMVS: error parsing -getroot output. Could not find ROOT line.")
	return strings.group(1).strip()

def HasRoot():
	try:
		GetRoot()
		return True
	except:
		return False

def GetAlt(filename):
	startupinfo = subprocess.STARTUPINFO()
	startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
	p = subprocess.Popen([vimvs_exe, '-getalt="' + filename + '"'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, startupinfo=startupinfo)
	out,err = p.communicate()
	if p.returncode>0:
		return "";
	strings = re.search("^\s*ALT:(.*)", out, flags=re.MULTILINE)
	if strings is None:
		return ""
	return strings.group(1).strip()

def LoadQuickfix():
	with open(GetRoot() + ".vimvs.quickfix") as f:
		lines = f.readlines()
	qf = []
	#print lines
	# Format of the file is: File|Line|Col|Type|Code|Message
	for l in lines:
		tokens = l.split("|")
		# Remove the [A-Z] initial character from the code, since the 'nr' field for setqflist only expect a number
		code = tokens[4]
		if not code[0].isdigit():
			code = code[1:]
		qf.append(dict(
				filename=tokens[0].replace("\\", "/"),
				lnum=tokens[1], 
				type=tokens[3],
				nr=code,
				text=tokens[5]))
	vim.eval('setqflist(%s)' % qf)

