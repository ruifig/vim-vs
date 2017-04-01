#pragma once

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)


#if NDEBUG
	#define CZ_ASSERT(expression) ((void)0)
	#define CZ_ASSERT_F(expression, fmt, ...) ((void)0)
	#define CZ_UNEXPECTED() ::cz::_doAssert(__FILE__, __LINE__, "Unexpected code path")
#else

/*! Checks if the expression is true/false and causes an assert if it's false.
 @hideinitializer
 Depending on the build configuration, asserts might be enabled for release build too
 */
	#define CZ_ASSERT(expression) if (!(expression)) { ::cz::_doAssert(__FILE__, __LINE__, #expression); }
	//#define CZ_ASSERT(expression) (void(0))

/*! Checks if the expression is true/false and causes an assert if it's false.
@hideinitializer
The difference between this and \link CZ_ASSERT \endlink is that it's suitable to display meaningful messages.
\param expression Expression to check
\param fmt printf style format string
*/
	#define CZ_ASSERT_F(expression, fmt, ...) if (!(expression)) { ::cz::_doAssert(__FILE__, __LINE__, fmt, ##__VA_ARGS__); } // By using ##__VA_ARGS__ , it will remove the last comma, if __VA_ARGS__ is empty

	#define CZ_UNEXPECTED() ::cz::_doAssert(__FILE__, __LINE__, "Unexpected code path")
	#define CZ_UNEXPECTED_F(fmt, ...) ::cz::_doAssert(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#endif

/*! Evaluates the expression, and asserts if asserts are enabled.
 @hideinitializer
 Note that even if asserts are disabled, it still evaluates the expression (it's not compiled-out like the standard 'assert' for release builds),
 so can be used to check if for example a function returned a specific value:
 \code
 CZ_CHECK( doSomethingCritical()==true );
 \endcode
 */
#define CZ_CHECK(expression) if (!(expression)) { ::cz::_doAssert(__FILE__, __LINE__, #expression); }

namespace cz
{

inline std::string tolower(const std::string& s)
{
	std::string r;
	std::transform(s.begin(), s.end(), std::back_inserter(r), ::tolower);
	return r;
}

inline void tolower_inplace(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}

std::string getWin32Error(const char* funcname);

void _doAssert(const char* file, int line, _Printf_format_string_ const char* fmt, ...);

enum
{
	kTemporaryStringMaxNesting = 20,
	kTemporaryStringMaxSize = 16000
};

static char* getTemporaryString();

const char* formatString(_Printf_format_string_ const char* format, ...);
const char* formatStringVA(const char* format, va_list argptr);

void ensureTrailingSlash(std::string& str);
std::string removeTrailingSlash(std::string str);

std::string getCWD();

bool isExistingFile(const std::string& filename);

std::string getProcessPath(std::string* fname = nullptr);


// Canonicalizes a path (converts relative to absolute, and converts all '/' characters to '\'
// "root" is used to process relative paths. If it's not specified, it will assume the current working Directory
bool fullPath(std::string& dst, const std::string& path, std::string root);

std::string replace(const std::string& s, char from, char to);
std::string replace(const std::string& str, const std::string& from, const std::string& to);
std::pair<std::string, std::string> splitFolderAndFile(const std::string& str);
std::string getExtension(const std::string& fname, std::string* name = nullptr);
std::string removeQuotes(const std::string& str);
int levenshtein_distance(const std::string& s1, const std::string& s2);

//! Converts a string from UTF-8 to UTF-16.
std::wstring widen(const std::string& str);
//! Converts a string from UTF-16 to UTF-8.
std::string narrow(const std::wstring& str);

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

bool endsWith(const std::string& str, const std::string& ending);
bool endsWith(const std::string& str, const char* ending);

// \param dst
//		If not nullptr, it will contain the string with the specified begin stripped (if the function returned true)
bool beginsWith(const std::string& str, const std::string& begins, std::string* dst = nullptr);
bool beginsWith(const std::string& str, const char* begins, std::string* dst = nullptr);

int64_t hash(const std::string& s);
int64_t hash(const std::vector<std::string>& v);

template <class T, class MTX=std::mutex>
class Monitor
{
private:
	mutable T m_t;
	mutable MTX m_mtx;

public:
	using Type = T;
	Monitor() {}
	Monitor(T t_) : m_t(std::move(t_)) {}
	template <typename F>
	auto operator()(F f) const -> decltype(f(m_t))
	{
		std::lock_guard<std::mutex> hold{ m_mtx };
		return f(m_t);
	}
};

template<class T>
void moveAppend(std::vector<T>& src, std::vector<T>& dst)
{
	assert(&dst != &src);
	if (dst.empty())
	{
		dst = std::move(src);
	}
	else
	{
		dst.reserve(dst.size() + src.size());
		std::move(std::begin(src), std::end(src), std::back_inserter(dst));
	}
	src.clear();
}

} // namesapce cz

