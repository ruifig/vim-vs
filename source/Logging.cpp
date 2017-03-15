/********************************************************************
	CrazyGaze (http://www.crazygaze.com)
	Author : Rui Figueira
	Email  : rui@crazygaze.com
	
	purpose:
	
*********************************************************************/

#include "vimvsPCH.h"
#include "Logging.h"

#define LOG_TIME 1
#define LOG_VERBOSITY 1

namespace cz
{

CZ_DEFINE_LOG_CATEGORY(logDefault)

#if CZ_NO_LOGGING
LogCategoryLogNone logNone;
#endif

LogCategoryBase::LogCategoryBase(const wchar_t* name, LogVerbosity verbosity, LogVerbosity compileTimeVerbosity) : m_name(name)
, m_verbosity(verbosity)
, m_compileTimeVerbosity(compileTimeVerbosity)
{

}

void LogCategoryBase::setVerbosity(LogVerbosity verbosity)
{
	// Take into considering the minimum compiled verbosity
	m_verbosity = LogVerbosity( std::min((int)m_compileTimeVerbosity, (int)verbosity) );
}

LogOutput::SharedData* LogOutput::getSharedData()
{
	// This is thread safe, according to C++11 (aka: Magic Statics)
	static SharedData data;
	return &data;
}

LogOutput::LogOutput()
{
	auto data = getSharedData();
	auto lk = std::unique_lock<std::mutex>(data->mtx);
	data->outputs.push_back(this);
}

LogOutput::~LogOutput()
{
	auto data = getSharedData();
	auto lk = std::unique_lock<std::mutex>(data->mtx);
	data->outputs.erase(std::find(data->outputs.begin(), data->outputs.end(), this));
}

void LogOutput::logToAll(const wchar_t* file, int line, const LogCategoryBase* category, LogVerbosity verbosity, const wchar_t* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	const wchar_t* prefix = L"";
#if LOG_TIME	
	{
		time_t t = time(nullptr);
		struct tm d;
		localtime_s(&d, &t);
		#if LOG_VERBOSITY
			prefix = formatString(L"%02d:%02d:%02d: %s: ", d.tm_hour, d.tm_min, d.tm_sec, logVerbosityToString(verbosity));
		#else
			prefix = formatString(L"%02d:%02d:%02d: ", d.tm_hour, d.tm_min, d.tm_sec);
		#endif
	}
#else
	prefix = formatString(L"%s: ", logVerbosityToString(verbosity));
#endif

	auto msg = formatStringVA(fmt, args);
	wchar_t buf[1024];
	const int s = sizeof(buf)*sizeof(buf[0]);
	auto count = _snwprintf_s(buf, s, L"%s%s", prefix, msg);
	if (!(count>=0 && count<s))
		buf[sizeof(buf) - 1] = 0;

	auto data = getSharedData();
	auto lk = std::unique_lock<std::mutex>(data->mtx);
	for (auto&& out : data->outputs)
	{
		out->log(file, line, category, verbosity, buf);
	}
}


}
