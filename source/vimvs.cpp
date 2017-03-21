#include "vimvsPCH.h"
#include "Parser.h"
#include "Parameters.h"
#include "IniFile.h"
#include "Logging.h"
#include "ChildProcessLauncher.h"
#include "ScopeGuard.h"
#include "SqLiteWrapper.h"
#include "UTF8String.h"

#define VIMVS_CFG_FILE ".vimvs_conf.ini"
#define VIMVS_LOG_FILE ".vimvs.log"
#define VIMVS_DB_FILE ".vimvs.sqlite"

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
	virtual void log(const char* file, int line, const LogCategoryBase* category, LogVerbosity verbosity, const char* msg) override
	{
		printf("%s\n", msg);
	}
};

class FileLogger : LogOutput
{
public:
	// According to "http://utf8everywhere.org/", Passing a char* to MSVC CRT will not treat it as UTF8, so we need to use
	// the wchar_t version
	FileLogger(const std::string& filename)
		: m_out(widen(filename), std::ofstream::out | std::ofstream::app)
		, m_filename(filename)
	{
	}

	bool isOpen() const
	{
		return m_out.is_open();
	}
	
	const std::string getFilename() const
	{
		return m_filename;
	}
private:

	virtual void log(const char* file, int line, const LogCategoryBase* category, LogVerbosity verbosity, const char* msg) override
	{
		m_out << msg << std::endl;
	}

	std::string m_filename;
	std::ofstream m_out;
};

struct Config
{
	std::string exeRoot;
	std::string root;
	std::string slnfile; // full path to the solution file to use
	std::unique_ptr<FileLogger> fileLogger;

	std::string getUtilityPath(const char* name, bool quote=false)
	{
		auto s = exeRoot + "../../bin/" + name;
		fullPath(s, s, "");
		if (!isExistingFile(s))
		{
			CZ_LOG(logDefault, Fatal, "vim-vs utility '%s' not found", name);
			return "";
		}
		if (quote)
			return std::string("\"") + s + "\"";
		else
			return s;
	}

	bool load()
	{
		exeRoot = getProcessPath();

		auto found = findConfigFile(root);
		if (!found)
		{
			fprintf(stderr, "Could not find a '%s' file in current folder or any of the parents\n", VIMVS_CFG_FILE);
			return false;
		}

		// Create/open our log file before doing anything else, since we know where the config file is
		fileLogger = std::make_unique<FileLogger>(root + VIMVS_LOG_FILE);
		if (!fileLogger->isOpen())
		{
			fprintf(stderr, "Could not open log file '%s'", fileLogger->getFilename().c_str());
			return false;
		}

		IniFile cfg;
		cfg.open((root + VIMVS_CFG_FILE).c_str());
		auto str = cfg.getValue<const char*>("General", "solution", "");
		if (!fullPath(slnfile, str, root) || !isExistingFile(slnfile))
		{
			CZ_LOG(logDefault, Fatal, "Invalid solution path (%s)", str);
			return false;
		}

		CZ_LOG(logDefault, Log, "Using solution file '%s'", slnfile.c_str());

		return true;
	}

	static bool findConfigFile(std::string& dir)
	{
		std::string d = getCWD();
		std::string previous;
		std::string f;
		do
		{
			ensureTrailingSlash(d);
			if (isExistingFile(d + VIMVS_CFG_FILE))
			{
				dir = d;
				return true;
			}
			previous = d;
		} while (fullPath(d, d + "..", "") && d != previous);
		return false;
	}
};

Parameters gParams(Parameters::Auto);
std::unique_ptr<Config> gCfg;
std::unique_ptr<Database> gDb;

std::string genParams(std::vector<std::string> p)
{
	std::string res;
	for (auto s : p)
	{
		res += "\"";
		res += s;
		res += "\" ";
	}
	return res;
}

int buildCompileDatabase()
{
	Parser parser(*gDb);

	CZ_LOG(logDefault, Log, "Generating compile database");

	ChildProcessLauncher launcher;
	auto exitCode = launcher.launch(
		gCfg->getUtilityPath("vim-vs.msbuild.bat"),
		genParams(
			{gCfg->slnfile,
			std::string("/p:ForceImportBeforeCppTargets=")+gCfg->getUtilityPath("gen.props", true),
			"/t:Rebuild",
			"/maxcpucount"
		}),
		[&](bool iscmdline, const std::string& str)
	{
		if (iscmdline)
		{
			CZ_LOG(logDefault, Log, "msbuild command line: %s\n", str.c_str());
		}
		else
		{
			parser.inject(str);
		}
	});

	if (exitCode)
	{
		CZ_LOG(logDefault, Error, "build failed");
		printf(launcher.getFullOutput().c_str());
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
	CZ_LOG(logDefault, Log, "Common clang params: %s", commonParams.c_str());

	std::ofstream out(widen(gCfg->root + "compile_commands.json"));
	using namespace nlohmann;
	json j;
	for (auto&& f : gDb->files())
	{
		auto ff = splitFolderAndFile(f.second.name);
		j.push_back({
			{"directory", ff.first},
			{"command", commonParams + f.second.systemIncludes->getIncludes() + f.second.params->getReadyParams()},
			{"file", f.second.name}
			});
	}
	out << std::setw(4) << j << std::endl;

	return EXIT_SUCCESS;
}

void testdatabase()
{
	Database db;
	CZ_CHECK(db.open(gCfg->root + VIMVS_DB_FILE));
}

int teststring()
{
	UTF8String s1("Hello -\u20AC-");
	UTF8String s2(std::string("Hello"));

	auto w = s2.widen();

	SetConsoleOutputCP(65001);
	SetConsoleCP(65001);
	printf("%s\n", s1.c_str());

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

	gDb = std::make_unique<Database>();
	SCOPE_EXIT{ gDb.reset(); };
	if (!gDb->open(gCfg->root + VIMVS_DB_FILE))
		return EXIT_FAILURE;

	if (gParams.has("generate_compile_database"))
		return buildCompileDatabase();
	
	if (gParams.has("testdatabase"))
		return testdatabase();

	if (gParams.has("teststring"))
		return teststring();

	return EXIT_SUCCESS;
}
