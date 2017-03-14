/********************************************************************
	CrazyGaze (http://www.crazygaze.com)
	Author : Rui Figueira
	Email  : rui@crazygaze.com
	
	purpose:
	Code to launch child processes and capture their output	
*********************************************************************/

#pragma once

#include <string>

namespace cz {
namespace apcpuide {
namespace common {

// Child process launcher based on http://support.microsoft.com/kb/190351
class ChildProcessLauncher
{
	enum
	{
		BUFSIZE=4096
	};

public:
	ChildProcessLauncher();
	~ChildProcessLauncher();
	int launch(const UTF8String& name, const UTF8String& params, const std::function<void(bool, const cz::UTF8String& str)>& logfunc=nullptr);
	int getExitCode();
	const UTF8String& getLaunchErrorMsg()
	{
		return m_errmsg;
	}

private:
	int PrepAndLaunchRedirectedChild(HANDLE hChildStdOut, HANDLE hChildStdIn, HANDLE hChildStdErr);
	int ReadAndHandleOutput(HANDLE hPipeRead);
	int ErrorMessage(PTSTR lpszFunction);
	void addOutput(const char* str, int nbytesread);
	UTF8String m_errmsg;
	UTF8String m_name;
	HANDLE m_hStdIn;
	HANDLE m_hChildProcess;
	BOOL m_bRunThread;
	UTF8String m_params;
	UTF8String m_output;
	std::function<void(bool isLaunchCmd, const cz::UTF8String& str)> m_logfunc;
};

} // namespace common
} // namespace apcpuide
} // namespace cz

