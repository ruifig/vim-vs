#include "vimvsPCH.h"
#include "Utils.h"
#include "Logging.h"
#include "ScopeGuard.h"

namespace cz
{

std::string getWin32Error(const char* funcname)
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	               NULL,
	               dw,
	               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	               (LPTSTR)&lpMsgBuf,
	               0,
	               NULL);
	SCOPE_EXIT{ LocalFree(lpMsgBuf); };

	int funcnameLength = funcname ? lstrlen((LPCTSTR)funcname) : 0;
	lpDisplayBuf =
	    (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + funcnameLength + 50) * sizeof(wchar_t));
	if (lpDisplayBuf == NULL)
		return "Win32ErrorMsg failed";
	SCOPE_EXIT{ LocalFree(lpDisplayBuf); };

	auto wfuncname = funcname ? widen(funcname) : L"";

	StringCchPrintfW((LPTSTR)lpDisplayBuf,
	                 LocalSize(lpDisplayBuf) / sizeof(wchar_t),
	                 L"%s failed with error %lu: %s",
	                 wfuncname.c_str(),
	                 dw,
	                 (LPTSTR)lpMsgBuf);

	std::wstring ret = (LPTSTR)lpDisplayBuf;

	// Remove the \r\n at the end
	while (ret.size() && ret.back() < ' ')
		ret.pop_back();

	assert(0);
	return narrow(ret);
}

void _doAssert(const char* file, int line, _Printf_format_string_ const char* fmt, ...)
{
	static bool executing;

	// Detect reentrancy, since we call a couple of things from here, that might end up asserting
	if (executing)
		__debugbreak();
	executing = true;

	char buf[1024];
	va_list args;
	va_start(args, fmt);
	_vsnprintf_s(buf, 1024, _TRUNCATE, fmt, args);
	va_end(args);

	CZ_LOG(logDefault, Fatal, "ASSERT: %s,%d: %s\n", file, line, buf);

	if (::IsDebuggerPresent())
	{
		__debugbreak(); // This will break in all builds
	}
	else
	{
		//_wassert(wbuf, wfile, line);
		//DebugBreak();
		__debugbreak(); // This will break in all builds
	}
}

char* getTemporaryString()
{
	// Use several static strings, and keep picking the next one, so that callers can hold the string for a while
	// without risk of it being changed by another call.
	__declspec(thread) static char bufs[kTemporaryStringMaxNesting][kTemporaryStringMaxSize];
	__declspec(thread) static char nBufIndex = 0;
	char* buf = bufs[nBufIndex];
	nBufIndex++;
	if (nBufIndex == kTemporaryStringMaxNesting)
		nBufIndex = 0;
	return buf;
}

const char* formatString(_Printf_format_string_ const char* format, ...)
{
	va_list args;
	va_start(args, format);
	const char* str = formatStringVA(format, args);
	va_end(args);
	return str;
}

const char* formatStringVA(const char* format, va_list argptr)
{
	char* buf = getTemporaryString();
	_vsnprintf_s(buf, kTemporaryStringMaxSize, _TRUNCATE, format, argptr);
	return buf;
}

void ensureTrailingSlash(std::string& str)
{
	if (str.size() && !(str[str.size() - 1] == '\\' || str[str.size() - 1] == '/'))
		str += '\\';
}

std::string getCWD()
{
	wchar_t buf[MAX_PATH];
	CZ_CHECK(GetCurrentDirectoryW(MAX_PATH, buf) != 0);
	return narrow(buf) + "\\";
}

bool isExistingFile(const std::string& filename)
{
	DWORD dwAttrib = GetFileAttributesW(widen(filename).c_str());
	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

std::string getProcessPath(std::string* fname)
{
	wchar_t buf[MAX_PATH];
	GetModuleFileNameW(NULL, buf, MAX_PATH);

	auto result = narrow(buf);
	std::string::size_type index = result.rfind("\\");

	if (index != std::string::npos)
	{
		if (fname)
			*fname = result.substr(index + 1);
		result = result.substr(0, index + 1);
	}
	else
		return "";

	return result;
}

bool fullPath(std::string& dst, const std::string& path, std::string root)
{
	wchar_t fullpathbuf[MAX_PATH];
	wchar_t srcfullpath[MAX_PATH];
	if (root.empty())
		root = getCWD();
	ensureTrailingSlash(root);

	auto wpath = widen(path);
	auto wroot = widen(root);

	std::wstring tmp = PathIsRelativeW(wpath.c_str()) ? wroot + wpath : wpath;
	wcscpy(srcfullpath, tmp.c_str());
	wchar_t* d = srcfullpath;
	wchar_t* s = srcfullpath;
	while (*s)
	{
		if (*s == '/')
			*s = '\\';
		*d++ = *s;

		// Skip any repeated separator
		if (*s == '\\')
		{
			s++;
			while (*s && (*s == '\\' || *s == '/'))
				s++;
		}
		else
		{
			s++;
		}
	}
	*d = 0;

	bool res = PathCanonicalizeW(fullpathbuf, srcfullpath) ? true : false;
	if (res)
		dst = narrow(fullpathbuf);
	return res;
}

std::string replace(const std::string& s, char from, char to)
{
	std::string res = s;
	for (auto&& ch : res)
	{
		if (ch == from)
			ch = to;
	}
	return res;
}

std::string replace(const std::string& str, const std::string& from, const std::string& to)
{
	if (from.empty())
		return "";
	size_t start_pos = 0;
	auto res = str;
	while ((start_pos = res.find(from, start_pos)) != std::string::npos)
	{
		res.replace(start_pos, from.length(), to);
		start_pos += to.length();  // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
	return res;
}

std::pair<std::string, std::string> splitFolderAndFile(const std::string& str)
{
	auto i = std::find_if(str.rbegin(), str.rend(), [](const char& ch)
	{
		return ch == '/' || ch == '\\';
	});

	std::pair < std::string, std::string> res;
	res.first = std::string(str.begin(), i.base());
	res.second = std::string(i.base(), str.end());
	return res;
}

std::string removeQuotes(const std::string& str)
{
	std::string res = str;
	while (res.size() && (res[0] == '"' || res[0] == '\''))
		res.erase(res.begin());
	while (res.size() && (res.back() == '"' || res.back() == '\''))
		res.pop_back();
	return res;
}

std::wstring widen(const std::string& utf8)
{
	if (utf8.empty())
		return std::wstring();

	// Get length (in wchar_t's), so we can reserve the size we need before the
	// actual conversion
	const int length = ::MultiByteToWideChar(CP_UTF8,             // convert from UTF-8
		0,                   // default flags
		utf8.data(),         // source UTF-8 string
		(int)utf8.length(),  // length (in chars) of source UTF-8 string
		NULL,                // unused - no conversion done in this step
		0                    // request size of destination buffer, in wchar_t's
	);
	if (length == 0)
		throw std::exception("Can't get length of UTF-16 string");

	std::wstring utf16;
	utf16.resize(length);

	// Do the actual conversion
	if (!::MultiByteToWideChar(CP_UTF8,             // convert from UTF-8
		0,                   // default flags
		utf8.data(),         // source UTF-8 string
		(int)utf8.length(),  // length (in chars) of source UTF-8 string
		&utf16[0],           // destination buffer
		(int)utf16.length()  // size of destination buffer, in wchar_t's
	))
	{
		throw std::exception("Can't convert string from UTF-8 to UTF-16");
	}

	return utf16;
}


std::string narrow(const std::wstring& str)
{
	if (str.empty())
		return std::string();

	// Get length (in wchar_t's), so we can reserve the size we need before the
	// actual conversion
	const int utf8_length = ::WideCharToMultiByte(CP_UTF8,              // convert to UTF-8
		0,                    // default flags
		str.data(),           // source UTF-16 string
		(int)str.length(),  // source string length, in wchar_t's,
		NULL,                 // unused - no conversion required in this step
		0,                    // request buffer size
		NULL,
		NULL  // unused
	);

	if (utf8_length == 0)
		throw "Can't get length of UTF-8 string";

	std::string utf8;
	utf8.resize(utf8_length);

	// Do the actual conversion
	if (!::WideCharToMultiByte(CP_UTF8,              // convert to UTF-8
		0,                    // default flags
		str.data(),           // source UTF-16 string
		(int)str.length(),    // source string length, in wchar_t's,
		&utf8[0],             // destination buffer
		(int)utf8.length(),   // destination buffer size, in chars
		NULL,
		NULL  // unused
	))
	{
		throw "Can't convert from UTF-16 to UTF-8";
	}

	return utf8;
}

bool isSpace(int a)
{
	return a == ' ' || a == '\t' || a == 0xA || a == 0xD;
}

bool notSpace(int a)
{
	return !isSpace(a);
}

bool endsWith(const std::string& str, const std::string& ending)
{
	if (str.length() >= ending.length()) {
		return (0 == str.compare(str.length() - ending.length(), ending.length(), ending));
	}
	else {
		return false;
	}
}

bool endsWith(const std::string& str, const char* ending)
{
	const size_t endingLength = strlen(ending);
	if (str.length() >= endingLength) {
		return (0 == str.compare(str.length() - endingLength, endingLength, ending));
	}
	else {
		return false;
	}
}

bool beginsWith(const std::string& str, const std::string& begins, std::string* dst)
{
	if (str.length() >= begins.length()) {
		bool res = (0 == str.compare(0, begins.length(), begins));
		if (res && dst)
			*dst = str.substr(begins.length());
		return res;
	}
	else {
		return false;
	}
}

bool beginsWith(const std::string& str, const char* begins, std::string* dst)
{
	const size_t beginsLength = strlen(begins);
	if (str.length() >= beginsLength) {
		bool res = (0 == str.compare(0, beginsLength, begins));
		if (res && dst)
			*dst = str.substr(beginsLength);
		return res;
	}
	else {
		return false;
	}
}

}
