/********************************************************************
	CrazyGaze (http://www.crazygaze.com)
	Author : Rui Figueira
	Email  : rui@crazygaze.com
	
	purpose:
	Logging functions/classes, somewhat inspired by how UE4 does it
	
*********************************************************************/

#pragma once

#include "Utils.h"

namespace cz
{

enum class LogVerbosity : uint8_t
{
	None, // Used internally only
	Fatal,
	Error,
	Warning,
	Log
};

static inline const wchar_t* logVerbosityToString(LogVerbosity v)
{
	switch (v)
	{
	case LogVerbosity::None   : return L"NNN";
	case LogVerbosity::Fatal  : return L"FTL";
	case LogVerbosity::Error  : return L"ERR";
	case LogVerbosity::Warning: return L"WRN";
	case LogVerbosity::Log    : return L"LOG";
	};
	return L"Unknown";
}

#define CZ_LOG_MINIMUM_VERBOSITY Log

class LogCategoryBase
{
public:
	LogCategoryBase(const wchar_t* name, LogVerbosity verbosity, LogVerbosity compileTimeVerbosity);
	__forceinline const std::wstring& getName() const
	{
		return m_name;
	}
	__forceinline bool isSuppressed(LogVerbosity verbosity) const
	{
		return verbosity > m_verbosity;
	}
	void setVerbosity(LogVerbosity verbosity);

protected:
	LogVerbosity m_verbosity;
	LogVerbosity m_compileTimeVerbosity;
	std::wstring m_name;
};

template<LogVerbosity DEFAULT, LogVerbosity COMPILETIME>
class LogCategory : public LogCategoryBase
{
public:
	LogCategory(const wchar_t* name) : LogCategoryBase(name, DEFAULT, COMPILETIME)
	{
	}

	// Compile time verbosity
	enum
	{
		CompileTimeVerbosity  = (int)COMPILETIME
	};

private:
};

class LogOutput
{
public:
	LogOutput();
	virtual ~LogOutput();
	static void logToAll(const wchar_t* file, int line, const LogCategoryBase* category, LogVerbosity verbosity, const wchar_t* fmt, ...);
private:
	virtual void log(const wchar_t* file, int line, const LogCategoryBase* category, LogVerbosity verbosity, const wchar_t* msg) = 0;

	struct SharedData
	{
		std::mutex mtx;
		std::vector<LogOutput*> outputs;
	};
	static SharedData* getSharedData();

};


#if CZ_NO_LOGGING

struct LogCategoryLogNone : public LogCategory<LogVerbosity::None, LogVerbosity::None>
{
	LogCategoryLogNone() : LogCategory("LogNone") {};
	void setVerbosity(LogVerbosity verbosity) {}
};
extern LogCategoryLogNone logNone;

#define CZ_DECLARE_LOG_CATEGORY(NAME, DEFAULT_VERBOSITY, COMPILETIME_VERBOSITY) extern ::cz::LogCategoryLogNone& NAME;
#define CZ_DEFINE_LOG_CATEGORY(NAME) ::cz::LogCategoryLogNone& NAME = ::cz::logNone;
#define CZ_LOG(NAME, VERBOSITY, fmt, ...)                               \
	{                                                                   \
		if (::cz::LogVerbosity::VERBOSITY == ::cz::LogVerbosity::Fatal) \
		{                                                               \
			::cz::_doAssert(__WFILE__, __LINE__, fmt, ##__VA_ARGS__);    \
		}                                                               \
	}

#else

#ifdef _MSC_VER
#pragma warning( disable : 4127 ) // warning C4127: conditional expression is constant
#endif

#define CZ_DECLARE_LOG_CATEGORY(NAME, DEFAULT_VERBOSITY, COMPILETIME_VERBOSITY) \
	extern class LogCategory##NAME : public ::cz::LogCategory<::cz::LogVerbosity::DEFAULT_VERBOSITY, ::cz::LogVerbosity::COMPILETIME_VERBOSITY> \
	{ \
		public: \
		LogCategory##NAME() : LogCategory(L#NAME) {} \
	} NAME;

#define CZ_DEFINE_LOG_CATEGORY(NAME) LogCategory##NAME NAME;

#define CZ_LOG_CHECK_COMPILETIME_VERBOSITY(NAME, VERBOSITY) \
	(((int)::cz::LogVerbosity::VERBOSITY <= LogCategory##NAME::CompileTimeVerbosity) && \
	 ((int)::cz::LogVerbosity::VERBOSITY <= (int)::cz::LogVerbosity::CZ_LOG_MINIMUM_VERBOSITY))

#define CZ_LOG(NAME, VERBOSITY, fmt, ...)                                                                \
	{                                                                                                    \
		if (CZ_LOG_CHECK_COMPILETIME_VERBOSITY(NAME, VERBOSITY))                                         \
		{                                                                                                \
			if (!NAME.isSuppressed(::cz::LogVerbosity::VERBOSITY))                                       \
			{                                                                                            \
				::cz::LogOutput::logToAll(__WFILE__, __LINE__, &NAME, ::cz::LogVerbosity::VERBOSITY, fmt, \
				                          ##__VA_ARGS__);                                                \
				if (::cz::LogVerbosity::VERBOSITY == ::cz::LogVerbosity::Fatal)                          \
				{                                                                                        \
					::cz::_doAssert(__WFILE__, __LINE__, fmt, ##__VA_ARGS__);                             \
				}                                                                                        \
			}                                                                                            \
		}                                                                                                \
	}

#endif

CZ_DECLARE_LOG_CATEGORY(logDefault, Log, Log)

} // namespace cz

