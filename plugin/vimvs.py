import os
import vim
import subprocess
import re

vimvs_exe = vim.eval("g:vimvs_exe")
vimvs_plugin_root = vim.eval("g:vimvs_plugin_root")

def ToVimString(s):
	return '"%s"' % s.replace('\\', '\\\\').replace('"', r'\"').replace('\'', r'\'')

def PrintError(e):
	vim.command("call vimvs#PrintError(%s)" % ToVimString(e))

def Launch(args):
	startupinfo = subprocess.STARTUPINFO()
	startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
	p = subprocess.Popen([vimvs_exe] + args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, startupinfo=startupinfo)
	out,err = p.communicate()
	if p.returncode>0:
		raise RuntimeError("VIMVS: %s" % err.strip())
		#raise RuntimeError("VIMVS: [%s] failed with: %s" % (' '.join(str(x) for x in args), err.strip()))
	return out

def GetRoot():
	out = Launch(['-getroot'])
	strings = re.search("^\s*ROOT:(.*)", out, flags=re.MULTILINE)
	if strings is None:
		raise RuntimeError("VIMVS: error parsing -getroot output. Could not find ROOT line.")
	return strings.group(1).strip()

def HasRoot():
	try:
		GetRoot()
		return True
	except:
		return False

def GetAlt(filename):
	out = Launch(['-getalt="' + filename + '"'])
	strings = re.search("^\s*ALT:(.*)", out, flags=re.MULTILINE)
	if strings is None:
		raise RuntimeError("VIMVS: error parsing -getalt output. Could not find ALT line.")
	return strings.group(1).strip()

def LoadQuickfix():
	fname = GetRoot() + ".vimvs.quickfix";
	if not os.path.exists(fname):
		return

	with open(fname) as f:
		lines = f.readlines()
	qf = []
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

