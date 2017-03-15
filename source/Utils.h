#pragma once

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)


#if NDEBUG
	#define CZ_ASSERT(expression) ((void)0)
	#define CZ_ASSERT_F(expression, fmt, ...) ((void)0)
	#define CZ_CHECK(expression) expression
	#define CZ_UNEXPECTED() ::cz::_doAssert(__WFILE__, __LINE__, "Unexpected code path")
#else

/*! Checks if the expression is true/false and causes an assert if it's false.
 @hideinitializer
 Depending on the build configuration, asserts might be enabled for release build too
 */
	#define CZ_ASSERT(expression) if (!(expression)) { ::cz::_doAssert(__WFILE__, __LINE__, L#expression); }
	//#define CZ_ASSERT(expression) (void(0))

/*! Checks if the expression is true/false and causes an assert if it's false.
@hideinitializer
The difference between this and \link CZ_ASSERT \endlink is that it's suitable to display meaningful messages.
\param expression Expression to check
\param fmt printf style format string
*/
	#define CZ_ASSERT_F(expression, fmt, ...) if (!(expression)) { ::cz::_doAssert(__WFILE__, __LINE__, fmt, ##__VA_ARGS__); } // By using ##__VA_ARGS__ , it will remove the last comma, if __VA_ARGS__ is empty

/*! Evaluates the expression, and asserts if asserts are enabled.
 @hideinitializer
 Note that even if asserts are disabled, it still evaluates the expression (it's not compiled-out like the standard 'assert' for release builds),
 so can be used to check if for example a function returned a specific value:
 \code
 CZ_CHECK( doSomethingCritical()==true );
 \endcode
 */
	#define CZ_CHECK(expression) if (!(expression)) { ::cz::_doAssert(__WFILE__, __LINE__, L#expression); }

	#define CZ_UNEXPECTED() ::cz::_doAssert(__WFILE__, __LINE__, "Unexpected code path")
	#define CZ_UNEXPECTED_F(fmt, ...) ::cz::_doAssert(__WFILE__, __LINE__, fmt, ##__VA_ARGS__)
#endif

namespace cz
{

void _doAssert(const wchar_t* file, int line, const wchar_t* fmt, ...);

enum
{
	kTemporaryStringMaxNesting = 20,
	kTemporaryStringMaxSize = 16000
};

static wchar_t* getTemporaryString();

const wchar_t* formatString(const wchar_t* format, ...);
const wchar_t* formatStringVA(const wchar_t* format, va_list argptr);

void ensureTrailingSlash(std::wstring& str);

std::wstring getCWD();

bool isExistingFile(const std::wstring& filename);

std::wstring getProcessPath(std::wstring* fname = nullptr);


// Canonicalizes a path (converts relative to absolute, and converts all '/' characters to '\'
// "root" is used to process relative paths. If it's not specified, it will assume the current working Directory
bool fullPath(std::wstring& dst, const std::wstring& path, std::wstring root);

std::wstring replace(const std::wstring& s, wchar_t from, wchar_t to);
std::wstring replace(const std::wstring& str, const std::wstring& from, const std::wstring& to);
std::pair<std::wstring, std::wstring> splitFolderAndFile(const std::wstring& str);

//
// Converts a string from UTF-8 to UTF-16.
//
std::wstring toUTF16(const std::string& utf8);

std::string toUTF8(const std::wstring& utf16);

bool isSpace(int a);
bool notSpace(int a);

// trim from start
template<class StringType>
static inline StringType ltrim(const StringType &s_) {
	StringType s = s_;
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
	return s;
}

// trim from end
template<class StringType>
static inline StringType rtrim(const StringType &s_) {
	StringType s = s_;
	s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
	return s;
}

// trim from both ends
template<class StringType>
static inline StringType trim(const StringType &s) {
	return ltrim(rtrim(s));
}

//
// Converts a string from UTF-8 to UTF-16.
//
std::wstring widen(const std::string& utf8);

bool endsWith(const std::wstring& str, const std::wstring& ending);
bool endsWith(const std::wstring& str, const wchar_t* ending);
bool beginsWith(const std::wstring& str, const std::wstring& begins);
bool beginsWith(const std::wstring& str, const wchar_t* begins);

} // namesapce cz

