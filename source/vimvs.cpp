#include "vimvsPCH.h"
#include "Parser.h"
#include "Parameters.h"
#include "IniFile.h"
#include "Logging.h"
#include "ChildProcessLauncher.h"
#include "ScopeGuard.h"


#define VIMVS_CFG_FILE L".vimvs_conf.ini"
#define VIMVS_LOG_FILE L".vimvs.log"

/*
Links with information about msbuild
	https://msdn.microsoft.com/en-us/library/ms164311.aspx
Good regular expression builder
	//https://regex101.com/
*/

namespace cz
{

class ConsoleLogger : LogOutput
{
private:
	virtual void log(const wchar_t* file, int line, const LogCategoryBase* category, LogVerbosity verbosity, const wchar_t* msg)
	{
		wprintf(L"%s\n", msg);
	}
};

class FileLogger : LogOutput
{
public:
	FileLogger(const std::wstring& filename)
		: m_out(filename, std::ofstream::out | std::ofstream::app)
		, m_filename(filename)
	{
	}

	bool isOpen() const
	{
		return m_out.is_open();
	}
	
	const std::wstring getFilename() const
	{
		return m_filename;
	}
private:

	virtual void log(const wchar_t* file, int line, const LogCategoryBase* category, LogVerbosity verbosity, const wchar_t* msg)
	{
		m_out << toUTF8(msg) << std::endl;
	}

	std::wstring m_filename;
	std::ofstream m_out;
};

struct Config
{
	std::wstring exeRoot;
	std::wstring root;
	std::wstring slnfile; // full path to the solution file to use
	std::unique_ptr<FileLogger> fileLogger;

	std::wstring getUtilityPath(const wchar_t* name, bool quote=false)
	{
		auto s = exeRoot + L"../../bin/" + name;
		fullPath(s, s, L"");
		if (!isExistingFile(s))
		{
			CZ_LOG(logDefault, Fatal, L"vim-vs utility '%s' not found", name);
			return L"";
		}
		if (quote)
			return std::wstring(L"\"") + s + L"\"";
		else
			return s;
	}

	bool load()
	{
		exeRoot = getProcessPath();

		auto found = findConfigFile(root);
		if (!found)
		{
			fprintf(stderr, "Could not find a '%s' file in current folder or any of the parents\n", toUTF8(VIMVS_CFG_FILE).c_str());
			return false;
		}

		// Create/open our log file before doing anything else, since we know where the config file is
		fileLogger = std::make_unique<FileLogger>(root + VIMVS_LOG_FILE);
		if (!fileLogger->isOpen())
		{
			fprintf(stderr, "Could not open log file '%s'", toUTF8(fileLogger->getFilename()).c_str());
			return false;
		}

		IniFile cfg;
		cfg.open((root + VIMVS_CFG_FILE).c_str());
		auto str = cfg.getValue<const wchar_t*>(L"General", L"solution", L"");
		if (!fullPath(slnfile, str, root) || !isExistingFile(slnfile))
		{
			CZ_LOG(logDefault, Fatal, L"Invalid solution path (%s)", str);
			return false;
		}

		CZ_LOG(logDefault, Log, L"Using solution file '%s'", slnfile.c_str());

		return true;
	}

	static bool findConfigFile(std::wstring& dir)
	{
		std::wstring d = getCWD();
		std::wstring previous;
		std::wstring f;
		do
		{
			ensureTrailingSlash(d);
			if (isExistingFile(d + VIMVS_CFG_FILE))
			{
				dir = d;
				return true;
			}
			previous = d;
		} while (fullPath(d, d + L"..", L"") && d != previous);
		return false;
	}
};

Parameters gParams(Parameters::Auto);
std::unique_ptr<Config> gCfg;

std::wstring genParams(std::vector<std::wstring> p)
{
	std::wstring res;
	for (auto s : p)
	{
		res += L"\"";
		res += s;
		res += L"\" ";
	}
	return res;
}

int buildCompileDatabase()
{
	Database db;
	Parser parser(db);

	CZ_LOG(logDefault, Log, L"Generating compile database");

	ChildProcessLauncher launcher;
	auto exitCode = launcher.launch(
		gCfg->getUtilityPath(L"vim-vs.msbuild.bat"),
		genParams(
			{gCfg->slnfile,
			std::wstring(L"/p:ForceImportBeforeCppTargets=")+gCfg->getUtilityPath(L"gen.props", true),
			L"/t:Rebuild"
			, L"/maxcpucount"
		}),
		[&](bool iscmdline, const std::wstring& str)
	{
		if (iscmdline)
		{
			CZ_LOG(logDefault, Log, L"msbuild command line: %s\n", str.c_str());
		}
		else
		{
			parser.inject(str);
		}
	});

	if (exitCode)
	{
		CZ_LOG(logDefault, Error, L"build failed");
		wprintf(launcher.getFullOutput().c_str());
		//return EXIT_FAILURE;
	}

	std::string commonParams;
	commonParams += "-std=c++14 ";
	commonParams += "-x ";
	commonParams += "c++ ";
	commonParams += "-Wall ";
	commonParams += "-Wextra ";
	commonParams += "-fexceptions ";
	commonParams += "-DCINTERFACE "; // To let Clang parse VS's combaseapi.h, otherwise we get an error "unknown type name 'IUnknown'
	CZ_LOG(logDefault, Log, L"Common clang params: %s", toUTF16(commonParams).c_str());

	std::ofstream out(gCfg->root + L"compile_commands.json");
	using namespace nlohmann;
	json j;
	for (auto&& f : db.files())
	{
		auto ff = splitFolderAndFile(f.second.name);
		j.push_back({
			{"directory", toUTF8(ff.first)},
			{"command", commonParams + f.second.systemIncludes->getIncludes() + f.second.params->getReadyParams()},
			{"file", toUTF8(f.second.name)}
			});
	}
	out << std::setw(4) << j << std::endl;

	return EXIT_SUCCESS;
}

} // namespace cz

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	using namespace cz;
	ConsoleLogger logger;
	gCfg = std::make_unique<Config>();
	SCOPE_EXIT{ gCfg.reset(); };
	if (!gCfg->load())
		return EXIT_FAILURE;

	if (gParams.has(L"generate_compile_database"))
		return buildCompileDatabase();

	return EXIT_SUCCESS;
}
