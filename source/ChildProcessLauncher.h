/********************************************************************
	CrazyGaze (http://www.crazygaze.com)
	Author : Rui Figueira
	Email  : rui@crazygaze.com
	
	purpose:
	Code to launch child processes and capture their output	
*********************************************************************/

#pragma once

namespace cz {

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
	int launch(const std::wstring& name, const std::wstring& params, const std::function<void(bool, const std::wstring& str)>& logfunc=nullptr);

	const std::wstring& getLaunchErrorMsg()
	{
		return m_errmsg;
	}

private:
	int PrepAndLaunchRedirectedChild(HANDLE hChildStdOut, HANDLE hChildStdIn, HANDLE hChildStdErr);
	int ReadAndHandleOutput(HANDLE hPipeRead);
	int ErrorMessage(PTSTR lpszFunction);
	void addOutput(const wchar_t* str, int nbytesread);
	std::wstring m_errmsg;
	std::wstring m_name;
	HANDLE m_hStdIn;
	HANDLE m_hChildProcess;
	BOOL m_bRunThread;
	std::wstring m_params;
	std::wstring m_output;
	std::function<void(bool isLaunchCmd, const std::wstring& str)> m_logfunc;
};

} // namespace cz

