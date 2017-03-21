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

LogCategoryBase::LogCategoryBase(const char* name, LogVerbosity verbosity, LogVerbosity compileTimeVerbosity) : m_name(name)
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

void LogOutput::logToAll(const char* file, int line, const LogCategoryBase* category, LogVerbosity verbosity, _Printf_format_string_ const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	const char* prefix = "";
#if LOG_TIME	
	{
		time_t t = time(nullptr);
		struct tm d;
		localtime_s(&d, &t);
		#if LOG_VERBOSITY
			prefix = formatString("%02d:%02d:%02d: %s: ", d.tm_hour, d.tm_min, d.tm_sec, logVerbosityToString(verbosity));
		#else
			prefix = formatString("%02d:%02d:%02d: ", d.tm_hour, d.tm_min, d.tm_sec);
		#endif
	}
#else
	prefix = formatString("%s: ", logVerbosityToString(verbosity));
#endif

	auto msg = formatStringVA(fmt, args);
	char buf[1024];
	_snprintf_s(buf, _countof(buf), _TRUNCATE, "%s%s", prefix, msg);

	auto data = getSharedData();
	auto lk = std::unique_lock<std::mutex>(data->mtx);
	for (auto&& out : data->outputs)
	{
		out->log(file, line, category, verbosity, buf);
	}
}


}
