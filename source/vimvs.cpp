#include "vimvsPCH.h"
#include "Parser.h"
#include "Parameters.h"
#include "IniFile.h"
#include "Logging.h"
#include "ChildProcessLauncher.h"
#include "ScopeGuard.h"
#include "SqLiteWrapper.h"
#include "BuildGraph.h"

#define VIMVS_CFG_FILE ".vimvs_conf.ini"
#define VIMVS_LOG_FILE ".vimvs.log"
#define VIMVS_MSBUILDLOG_FILE ".vimvs.msbuild.log"
#define VIMVS_QUICKFIX_FILE ".vimvs.quickfix"
#define VIMVS_DB_FILE ".vimvs.sqlite"

//
// -DCINTERFACE
//		To let Clang parse VS's combaseapi.h, otherwise we get an error "unknown type name 'IUnknown'
// -x c++
//		Tell clang to use c++ if it can't figure out the type by the extension.
//		For example, system headers have no extension, but still need to be parsed as c++
//
#define VIMVS_FIXED_YCM_PARAMS "-DCINTERFACE|-x|c++|"

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
	virtual void log(const char* /*file*/, int line, const LogCategoryBase* category, LogVerbosity verbosity, const char* msg) override
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

	std::ofstream m_out;
	std::string m_filename;
};

struct Config
{
	std::string exeRoot;
	std::string root;
	std::string slnfile; // full path to the solution file to use
	bool fastParser = true;
	std::unique_ptr<FileLogger> fileLogger;
	std::string commonYcmParams;

	std::string getUtilityPath(const char* name, bool quote=false)
	{
		// First look in the the process's directory
		auto s = exeRoot + name;
		if (!isExistingFile(s))
			s = exeRoot + "../../bin/" + name;

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
			fprintf(stderr, "Could not find a '%s' file in current working directory or any of the parents\n", VIMVS_CFG_FILE);
			return false;
		}

		// Create/open our log file before doing anything else, since we know where the config file is
		fileLogger = std::make_unique<FileLogger>(root + VIMVS_LOG_FILE);
		if (!fileLogger->isOpen())
		{
			fprintf(stderr, "Could not open log file '%s'\n", fileLogger->getFilename().c_str());
			return false;
		}

		IniFile cfg;
		cfg.open((root + VIMVS_CFG_FILE).c_str());
		std::string str = cfg.getValue<const char*>("General", "solution", "");
		if (!fullPath(slnfile, str, root) || !isExistingFile(slnfile))
		{
			CZ_LOG(logDefault, Fatal, "Invalid solution path (%s)", str.c_str());
			fprintf(stderr, "Invalid solution path (%s)\n", str.c_str());
			return false;
		}
		CZ_LOG(logDefault, Log, "Using solution file '%s'", slnfile.c_str());

		str = cfg.getValue<const char*>("General", "common_ycm_params", "");
		commonYcmParams = VIMVS_FIXED_YCM_PARAMS;
		if (str.size())
		{
			commonYcmParams += str;
		}
		else
		{
			CZ_LOG(logDefault, Log, "common_ycm_params option not found in the config file");
		}
		CZ_LOG(logDefault, Log, "Using '%s' as common_ycm_params", commonYcmParams.c_str());

		fastParser = cfg.getValue<bool>("General", "fast_parser", true);

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


struct Cmd
{
	const char* cmd;
	bool(*func)(const Cmd&, const std::string&);
	const char* help;
};

struct Options
{
	std::string configuration;
	std::string platform;
};
Options gOptions;

bool cmd_help(const Cmd& cmd, const std::string& val);

bool cmd_getroot(const Cmd& cmd, const std::string& val)
{
	printf("ROOT:%s\n", gCfg->root.c_str());
	return true;
}

bool cmd_getycm(const Cmd& cmd, const std::string& val)
{
	std::string out;
	bool res;
	auto v = val;
	fullPath(v, val, getCWD());

	SourceFile f = gDb->getFile(v);
	if (!f.id)
	{
		out = "Not found";
		res = false;
	}
	else
	{
		out = formatString("YCM_CMD:|%s|%s|%s", gCfg->commonYcmParams.c_str(), f.defines.c_str(), f.includes.c_str());
		res = true;
	}

	CZ_LOG(logDefault, Log, "%s=%s", res ? "Success" : "Error", out.c_str());
	printf("%s\n", out.c_str());
	return res;
}

bool cmd_getalt(const Cmd& cmd, const std::string& val)
{
	auto v = removeQuotes(val);
	fullPath(v, v, getCWD());
	static std::vector<const char*> sources = { "cpp", "c", "cc", "c++", "cxx"};
	static std::vector<const char*> headers = { "h", "hh", "hxx", "hpp", "h++", "inl"};
	auto isIn = [](auto&& ext, auto&& c) -> bool
	{
		for (auto&& e : c)
		{
			if (ext == e)
				return true;
		}
		return false;
	};

	auto ext = tolower(getExtension(v));
	if (!isIn(ext, sources) && !isIn(ext, headers))
	{
		auto msg = formatString( "File '%s' doesn't have a known source/header extension", v.c_str());
		CZ_LOG(logDefault, Error, msg);
		fprintf(stderr,"%s\n", msg);
		return false;
	}

	SourceFile src = gDb->getFile(v);
	if (!src.id)
	{
		auto msg = formatString(
			"File '%s' not found in the database. Do a full build (-builddb) first to update the database",
			v.c_str());
		CZ_LOG(logDefault, Error, msg);
		fprintf(stderr,"%s\n", msg);
		return false;
	}

	auto basename = src.name;
	ext = tolower(getExtension(basename, &basename));

	const std::vector<const char*>* altext;
	if (isIn(ext, sources))
		altext = &headers;
	else if (isIn(ext, headers))
		altext = &sources;
	else
	{
		const char* out = "File extension not recognized as a C/C++ extension";
		CZ_LOG(logDefault, Log, out);
		fprintf(stderr, "%s\n", out);
		return false;
	}

	std::vector<SourceFile> alts;
	for (auto&& e : *altext)
	{
		auto res = gDb->getWithBasename(basename + "." + e);
		alts.insert(alts.end(), res.begin(), res.end());
	}

	std::vector<std::pair<int,std::string>> dists;
	for (auto&& a : alts)
		dists.emplace_back(levenshtein_distance(src.fullpath, a.fullpath), a.fullpath);
	std::sort(
		dists.begin(), dists.end(),
		[](auto&& a, auto && b)
		{
			return a.first < b.first;
		});
	
	if (!dists.size())
	{
		const char* out = "No alt file found\n";
		CZ_LOG(logDefault, Log, out);
		fprintf(stderr, "%s\n", out);
		return false;
	}

	printf("ALT:%s\n", dists[0].second.c_str());
	CZ_LOG(logDefault, Log, "ALT:%s", dists[0].second.c_str());
	for (auto it = dists.begin() + 1; it < dists.end(); ++it)
	{
		CZ_LOG(logDefault, Log, "OTHER:%s", it->second.c_str());
		printf("OTHER:%s\n", it->second.c_str());
	}

	return true;
}

// Good tips on how invoke msbuild to build, clean, rebuild a specific project
// http://stackoverflow.com/questions/13915636/specify-project-file-of-a-solution-using-msbuild
// http://stackoverflow.com/questions/9285756/how-do-i-compile-a-single-source-file-within-an-msvc-project-from-the-command-li
bool cmd_build(const Cmd& cmd, const std::string& val)
{
	bool builddb = std::string(cmd.cmd) == "builddb";
	auto fastParser = gParams.has("fastparser");
	std::ofstream quickfix(widen(gCfg->root + VIMVS_QUICKFIX_FILE), std::ofstream::out);
	std::ofstream msbuildlog(widen(gCfg->root + VIMVS_MSBUILDLOG_FILE), std::ofstream::out);

	if (builddb)
		CZ_LOG(logDefault, Log, "Generating compile database");

	std::vector<std::string> launchParams;
	auto v = val;
	if (val == "")
	{
		launchParams.push_back(gCfg->slnfile);
	}
	else if (beginsWith(v, "prj:", &v))
	{
		launchParams.push_back(gCfg->slnfile);
		launchParams.push_back("/t:" + v);
	}
	else if (beginsWith(v, "file:", &v))
	{
		v = removeQuotes(v);
		SourceFile src = gDb->getFile(v);
		if (!src.id)
		{
			auto msg = formatString(
				"File '%s' not found in the database. Do a full build (-builddb) first to update the database",
				v.c_str());
			CZ_LOG(logDefault, Error, msg);
			fprintf(stderr, "%s\n", msg);
			return false;
		}

		launchParams.push_back(formatString("\"%s\"", src.prjFile.c_str()));
		launchParams.push_back("/t:clCompile");
		launchParams.push_back(formatString("/p:SelectedFiles=\"%s\"", src.fullpath.c_str()));
	}
	else
	{
		auto msg = formatString("Invalid -build parameter (%s)", val.c_str());
		CZ_LOG(logDefault, Error, msg);
		fprintf(stderr, "%s\n", msg);
		return false;
	}

	auto configuration = gParams.get("configuration");
	auto platform = gParams.get("platform");
	if (configuration != "")
		launchParams.push_back(formatString("/p:Configuration=\"%s\"", configuration.c_str()));
	if (platform != "")
		launchParams.push_back(formatString("/p:Platform=\"%s\"", platform.c_str()));

	Parser parser(*gDb, builddb, true, fastParser);
	if (builddb)
	{
		if (fastParser)
		{
			launchParams.push_back("/p:TrackFileAccess=false");
			launchParams.push_back(formatString("/p:CLToolExe=%s.exe", VIMVS_FAST_PARSER_TOOL));
			launchParams.push_back(formatString("/p:LIBToolExe=%s.exe", VIMVS_FAST_PARSER_TOOL));
			launchParams.push_back(formatString("/p:LINKToolExe=%s.exe", VIMVS_FAST_PARSER_TOOL));
			auto quotedExeRoot = "\"" + removeTrailingSlash(gCfg->exeRoot) + "\"";
			launchParams.push_back("/p:CLToolPath=" + quotedExeRoot);
			launchParams.push_back("/p:LIBToolPath=" + quotedExeRoot);
			launchParams.push_back("/p:LINKToolPath=" + quotedExeRoot);
			launchParams.push_back(std::string("/p:ForceImportBeforeCppTargets=") + gCfg->getUtilityPath("gen_fastparser.props", true));
		}
		else
		{
			launchParams.push_back(std::string("/p:ForceImportBeforeCppTargets=") + gCfg->getUtilityPath("gen.props", true));
		}
	}

	launchParams.push_back("/maxcpucount");

	ChildProcessLauncher launcher;
	auto exitCode = launcher.launch(
		gCfg->getUtilityPath("vim-vs.msbuild.bat"),
		genParams(launchParams),
		[&](bool iscmdline, const std::string& str)
	{
		if (iscmdline)
		{
			CZ_LOG(logDefault, Log, "msbuild command line: %s\n", str.c_str());
		}
		else
		{
			parser.inject(str);
			msbuildlog << str;
		}
	});

	for (auto&& e : parser.getErrors())
	{
		quickfix << formatString("%s|%d|%d|%s|%s|%s\n",
			e.file.c_str(), e.line, e.col, e.type.c_str(), e.code.c_str(), e.msg.c_str());
	}

	if (exitCode)
	{
		CZ_LOG(logDefault, Error, "build failed");
		//printf("%s\n", launcher.getFullOutput().c_str());
		fprintf(stderr, "VIMVS: Build failed\n");
		return false;
		//return EXIT_FAILURE;
	}

	return true;
}

Cmd gCmds[] =
{

{
"help", &cmd_help,
"\
Vim-vs by Rui Figueira (http://bitbucket.org/ruifig/vim-vs)\n\
\n\
-help\n\
Shows this help\n\
"
},

{
"getroot", &cmd_getroot,
"\
-getroot\n\
Gets the project root (aka: The folder where vimvs configuration file is located)\n\
"
},
{
"getycm", &cmd_getycm,
"\
-getycm=<FILE>\n\
Gets the command line required to parse the file FILE with YouCompleteMe\n\
"
},
{
"getalt", &cmd_getalt,
"\
-getalt=<FILE>\n\
Gets the file considered the best alternate (e.g: Given an header file, it returns the source file)\n\
"
},
{
"build", &cmd_build,
"\
-build[ = ( <prj:PROJECT> | <file:FILE> ) ]\n\
Builds the entire solution, one project, or compiles 1 single file\n\
Examples:\n\
-build\n\
	Build the entire solution\n\
-build=prj:Foo\n\
	Build the project 'Foo'\n\
-build=file:bar.cpp\n\
	Compiles the file 'bar.cpp'\n\
"
},
{
"builddb", &cmd_build,
"\
Same as '-build', but adds compile parameters to the sqlite database.\n\
"
},
{ nullptr, nullptr, nullptr }
};

bool cmd_help(const Cmd& cmd, const std::string& val)
{
	auto c = &gCmds[0];
	while (c->cmd)
	{
		printf("%s\n", c->help);
		c++;
	}

	return true;
}

} // namespace cz


int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	using namespace cz;

	// If we want to just show the help, then don't try to load the configuration
	if (!gParams.has("help"))
	{
		gCfg = std::make_unique<Config>();
		if (!gCfg->load())
			return EXIT_FAILURE;
		gDb = std::make_unique<Database>();
		if (!gDb->open(gCfg->root + VIMVS_DB_FILE))
			return EXIT_FAILURE;
	}
	SCOPE_EXIT{ gCfg.reset(); };
	SCOPE_EXIT{ gDb.reset(); };

	if (gParams.has("test"))
	{
		buildgraph::Graph graph;
		auto includeDirs = std::make_shared<buildgraph::IncludeDirs>();
		includeDirs->addI("C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/");
		includeDirs->addI("C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/atlmfc/include/");
		includeDirs->addI("C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/");
		includeDirs->addI("C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/um/");
		includeDirs->addI("C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/shared/");
		includeDirs->addI("C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/winrt/");
		includeDirs->addI("C:/Program Files (x86)/Windows Kits/NETFXSDK/4.6.2/Include/um/");
		includeDirs->addI("C:\\Work\\crazygaze\\vim-vs\\dummy\\..\\dummy\\d0\\d1\\d2\\d3\\");
		std::string f("C:\\Work\\crazygaze\\vim-vs\\dummy\\dummy.cpp");
		includeDirs->pushParent(splitFolderAndFile(f).first);
		graph.processIncludes(f, includeDirs, true);
		graph.finishWork();
		return EXIT_SUCCESS;
	}

	auto cmd = &gCmds[0];
	while (cmd->cmd)
	{
		if (gParams.has(cmd->cmd))
		{
			auto val = gParams.get(cmd->cmd);
			CZ_LOG(logDefault, Log, "Executing -%s=%s", cmd->cmd, val.c_str());
			return cmd->func(*cmd, val) ? EXIT_SUCCESS : EXIT_FAILURE;
		}
		cmd++;
	}

	return EXIT_SUCCESS;
}
