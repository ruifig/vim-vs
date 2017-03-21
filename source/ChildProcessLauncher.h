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
	int launch(const std::string& name, const std::string& params, const std::function<void(bool, const std::string& str)>& logfunc=nullptr);

	const std::string& getLaunchErrorMsg()
	{
		return m_errmsg;
	}

	const std::string& getFullOutput() const
	{
		return m_output;
	}

private:
	int PrepAndLaunchRedirectedChild(HANDLE hChildStdOut, HANDLE hChildStdIn, HANDLE hChildStdErr);
	int ReadAndHandleOutput(HANDLE hPipeRead);
	int ErrorMessage(const char* funcnam);
	void addOutput(const std::string& str);
	std::string m_errmsg;
	std::string m_name;
	HANDLE m_hStdIn;
	HANDLE m_hChildProcess;
	BOOL m_bRunThread;
	std::string m_params;
	std::string m_output;
	std::string m_tmpline;
	std::function<void(bool isLaunchCmd, const std::string& str)> m_logfunc;
};

} // namespace cz

