
#include "vimvsPCH.h"
#include "ChildProcessLauncher.h"
#include "Utils.h"

namespace cz {

ChildProcessLauncher::ChildProcessLauncher()
{
	m_hStdIn = NULL;
	m_hChildProcess = NULL;
	m_bRunThread = TRUE;
}

ChildProcessLauncher::~ChildProcessLauncher()
{
	/*
	std::vector<String> lines;
	cz::stringSplitIntoLines(m_output.c_str(), m_output.size(), &lines);
	for(auto &l : lines)
	{
		//
		// Simply doing  CZ_LOG(logDefault, Log, l.c_str())  will crash, if 'l' itself has any printf like stuff.
		// so, doing this thing below...
		CZ_LOG(logDefault, Log, "%s", l.c_str());
	}
	*/
}

int ChildProcessLauncher::ErrorMessage(PTSTR lpszFunction) 
{ 
	if (m_errmsg.size())
		return 1;

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    
	//MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

	m_errmsg = (wchar_t*)lpDisplayBuf;
    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    
	//ExitProcess(1);

	return 1;
}


void ChildProcessLauncher::addOutput(const wchar_t* str, int nbytesread)
{
	auto tmp = std::wstring(str, str+(nbytesread / sizeof(wchar_t)));
	m_output += tmp;
	if (m_logfunc)
	{
		m_logfunc(false, tmp);
	}
}

int ChildProcessLauncher::launch(const std::wstring& name, const std::wstring& params, const std::function<void(bool, const std::wstring& str)>& logfunc)
{
	m_name = name;
	m_params = params;
	m_logfunc = logfunc;

	HANDLE hOutputReadTmp, hOutputRead, hOutputWrite;
	HANDLE hInputWriteTmp, hInputRead, hInputWrite;
	HANDLE hErrorWrite;
	//HANDLE hThread;
	//DWORD ThreadId;
	SECURITY_ATTRIBUTES sa;

	// Set up the security attributes struct.
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	// Create the child output pipe.
	if (!CreatePipe(&hOutputReadTmp, &hOutputWrite, &sa, 0))
		ErrorMessage(TEXT("CreatePipe"));


	// Create a duplicate of the output write handle for the std error
	// write handle. This is necessary in case the child application
	// closes one of its std output handles.
	if (!DuplicateHandle(GetCurrentProcess(), hOutputWrite,
		GetCurrentProcess(), &hErrorWrite, 0,
		TRUE, DUPLICATE_SAME_ACCESS))
		ErrorMessage(TEXT("DuplicateHandle"));


	// Create the child input pipe.
	if (!CreatePipe(&hInputRead, &hInputWriteTmp, &sa, 0))
		ErrorMessage(TEXT("CreatePipe"));


	// Create new output read handle and the input write handles. Set
	// the Properties to FALSE. Otherwise, the child inherits the
	// properties and, as a result, non-closeable handles to the pipes
	// are created.
	if (!DuplicateHandle(GetCurrentProcess(), hOutputReadTmp,
		GetCurrentProcess(),
		&hOutputRead, // Address of new handle.
		0, FALSE, // Make it uninheritable.
		DUPLICATE_SAME_ACCESS))
		ErrorMessage(TEXT("DuplicateHandle"));

	if (!DuplicateHandle(GetCurrentProcess(), hInputWriteTmp,
		GetCurrentProcess(),
		&hInputWrite, // Address of new handle.
		0, FALSE, // Make it uninheritable.
		DUPLICATE_SAME_ACCESS))
		ErrorMessage(TEXT("DupliateHandle"));


	// Close inheritable copies of the handles you do not want to be
	// inherited.
	if (!CloseHandle(hOutputReadTmp)) ErrorMessage(TEXT("CloseHandle"));
	if (!CloseHandle(hInputWriteTmp)) ErrorMessage(TEXT("CloseHandle"));


	// Get std input handle so you can close it and force the ReadFile to
	// fail when you want the input thread to exit.
	if ((m_hStdIn = GetStdHandle(STD_INPUT_HANDLE)) ==
		INVALID_HANDLE_VALUE)
		ErrorMessage(TEXT("GetStdHandle"));

	PrepAndLaunchRedirectedChild(hOutputWrite, hInputRead, hErrorWrite);


	// Close pipe handles (do not continue to modify the parent).
	// You need to make sure that no handles to the write end of the
	// output pipe are maintained in this process or else the pipe will
	// not close when the child process exits and the ReadFile will hang.
	if (!CloseHandle(hOutputWrite)) ErrorMessage(TEXT("CloseHandle"));
	if (!CloseHandle(hInputRead)) ErrorMessage(TEXT("CloseHandle"));
	if (!CloseHandle(hErrorWrite)) ErrorMessage(TEXT("CloseHandle"));


	// Launch the thread that gets the input and sends it to the child.
	/*
	hThread = CreateThread(NULL, 0, GetAndSendInputThread,
		(LPVOID)hInputWrite, 0, &ThreadId);
	if (hThread == NULL) ErrorMessage(TEXT("CreateThread"));
	*/

	// Read the child's output.
	ReadAndHandleOutput(hOutputRead);
	// Redirection is complete


	// Force the read on the input to return by closing the stdin handle.
	// if (!CloseHandle(m_hStdIn)) ErrorMessage(TEXT("CloseHandle"));


	/*
	// Tell the thread to exit and wait for thread to die.
	m_bRunThread = FALSE;

	if (WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED)
		ErrorMessage(TEXT("WaitForSingleObject"));
		*/

	if (!CloseHandle(hOutputRead)) ErrorMessage(TEXT("CloseHandle"));
	if (!CloseHandle(hInputWrite)) ErrorMessage(TEXT("CloseHandle"));

	DWORD exitcode=0;
	if (!GetExitCodeProcess(m_hChildProcess, &exitcode))
		ErrorMessage(TEXT("GetExitCodeProcess"));

	if (!CloseHandle(m_hChildProcess))
		ErrorMessage(TEXT("CloseHandle"));

	if (m_errmsg.size())
		addOutput(m_errmsg.data(), static_cast<int>(m_errmsg.size() * sizeof(m_errmsg[0])));

	return m_errmsg.size() ? 1 : exitcode;
}

int ChildProcessLauncher::PrepAndLaunchRedirectedChild(
							HANDLE hChildStdOut,
							HANDLE hChildStdIn,
							HANDLE hChildStdErr)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;

	// Set up the start up info struct.
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdOutput = hChildStdOut;
	si.hStdInput = hChildStdIn;
	si.hStdError = hChildStdErr;
	// Use this if you want to hide the child:
	//     si.wShowWindow = SW_HIDE;
	// Note that dwFlags must include STARTF_USESHOWWINDOW if you want to
	// use the wShowWindow flags.

	std::wstring cmdline = std::wstring(L"\"") + m_name + L"\" " + m_params;

	m_logfunc(true, cmdline + L"\n");
	// Launch the process that you want to redirect (in this case,
	// Child.exe). Make sure Child.exe is in the same directory as
	// redirect.c launch redirect from a command line to prevent location
	// confusion.
	if (!CreateProcess(NULL, (LPWSTR)cmdline.c_str(), NULL, NULL, TRUE,
		CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
		ErrorMessage(TEXT("CreateProcess"));

	// Set global child process handle to cause threads to exit.
	m_hChildProcess = pi.hProcess;

	// Close any unnecessary handles.
	if (!CloseHandle(pi.hThread)) ErrorMessage(TEXT("CloseHandle"));

	return 0;
}

/////////////////////////////////////////////////////////////////////// 
// ReadAndHandleOutput
// Monitors handle for input. Exits when child exits or pipe breaks.
/////////////////////////////////////////////////////////////////////// 
int ChildProcessLauncher::ReadAndHandleOutput(HANDLE hPipeRead)
{
  CHAR lpBuffer[256];
  DWORD nBytesRead;
  //DWORD nCharsWritten;

  while(TRUE)
  {
	 if (!ReadFile(hPipeRead,lpBuffer,sizeof(lpBuffer),
									  &nBytesRead,NULL) || !nBytesRead)
	 {
		if (GetLastError() == ERROR_BROKEN_PIPE)
		   break; // pipe done - normal exit path.
		else
		   ErrorMessage(TEXT("ReadFile")); // Something bad happened.
	 }

	 /*
	 // Display the character read on the screen.
	 if (!WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),lpBuffer,
					   nBytesRead,&nCharsWritten,NULL))
		ErrorMessage(("WriteConsole"));
		*/
	 std::string s(lpBuffer, lpBuffer + nBytesRead);
	 addOutput(widen(s).c_str(), nBytesRead);

  }

  return 0;
}

} // namespace cz
