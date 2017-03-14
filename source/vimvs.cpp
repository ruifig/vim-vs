#include "vimvsPCH.h"
#include "Parser.h"
#include "Parameters.h"
#include "IniFile.h"
#include "Logging.h"
#include "ChildProcessLauncher.h"


#define VIMVS_CFG_FILE L".vimvs_conf.ini"
/*
Links with information about msbuild
	https://msdn.microsoft.com/en-us/library/ms164311.aspx
Good regular expression builder
	//https://regex101.com/
*/

using namespace cz;

class ConsoleLogger : LogOutput
{
private:
	virtual void log(const wchar_t* file, int line, const LogCategoryBase* category, LogVerbosity LogVerbosity, const wchar_t* msg)
	{
		wprintf(L"%s: %s", category->getName().c_str(), msg);
	}
};

struct Config
{
	std::wstring exeRoot;
	std::wstring root;
	std::wstring slnfile; // full path to the solution file to use

	std::wstring getUtilityPath(const wchar_t* name)
	{
		auto s = exeRoot + L"../../bin/" + name;
		fullPath(s, s, L"");
		if (!isExistingFile(s))
		{
			CZ_LOG(logDefault, Fatal, L"vim-vs utility '%s' not found", name);
			return L"";
		}
		return s;
	}

	bool load()
	{
		exeRoot = getProcessPath();

		auto found = findConfigFile(root);
		if (!found)
		{
			CZ_LOG(logDefault, Fatal, L"Could not find configuration file");
			return false;
		}

		CZ_LOG(logDefault, Log, L"Config file found at %s", root.c_str());

		IniFile cfg;
		cfg.open((root + VIMVS_CFG_FILE).c_str());
		auto str = cfg.getValue<const wchar_t*>(L"General", L"solution", L"");
		if (!fullPath(slnfile, str, root) || !isExistingFile(slnfile))
		{
			CZ_LOG(logDefault, Fatal, L"Invalid solution path (%s)", str);
			return false;
		}

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


void tests()
{
	std::wstring exename;
	auto p = getProcessPath(&exename);

	const int bufferLength = MAX_PATH;
	wchar_t buf[bufferLength + 1];
	// Change Current directory to where the executable is
	CZ_CHECK(GetModuleFileNameExW(GetCurrentProcess(), NULL, buf, bufferLength) != 0);

	ConsoleLogger logger;
	Config cfg;
	cfg.load();

	ChildProcessLauncher launcher;
	auto exitCode = launcher.launch(cfg.getUtilityPath(L"vim-vs.msbuild.bat"), L"/?",
		[](bool res, const std::wstring& str)
	{
		wprintf(str.c_str());
	});

	wprintf(L"Exit code = %d\n", exitCode);
	wprintf(L"Launch error message= %s\n", launcher.getLaunchErrorMsg().c_str());

}


int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	Parameters params(Parameters::Auto);
	tests();
	return EXIT_SUCCESS;


	Database db;
	Parser parser(db);

	std::wstring srcfile = params.Get(L"in");
	if (srcfile.size()==0)
		srcfile = L"../../data/showincludes_2.log";
	std::wstring dstfile = params.Get(L"out");
	if (dstfile.size()==0)
		dstfile = L"../../compile_commands.json";
	std::ifstream ifs(srcfile);
	CZ_CHECK(ifs.is_open());
	auto content = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
	ifs.close();
	parser.inject(widen(content));

	std::string commonParams;
	commonParams += "-std=c++14 ";
	commonParams += "-x ";
	commonParams += "c++ ";
	commonParams += "-Wall ";
	commonParams += "-Wextra ";
	commonParams += "-fexceptions ";
	commonParams += "-DCINTERFACE "; // To let Clang parse VS's combaseapi.h, otherwise we get an error "unknown type name 'IUnknown'
	// system includes

	std::ofstream out(dstfile);
	using namespace nlohmann;
	json j;
	for (auto&& f : db.files())
	{
		auto ff = splitFolderAndFile(f.second.name);
		j.push_back({
			{"directory", toUTF8(ff.first)},
			{"command", commonParams + f.second.systemIncludes->getIncludes() + f.second.params->getReadyParams()},
			{"file", toUTF8(f.second.name)}
			//,{"project", toUTF8(f.second.prjName)}
			});
		//out << "        \"directory\": \"" << json::escape_string(toUTF8(ff.first)) << "\"" << std::endl;
		//out << "        \"command\": \"" << json::escape_string("/usr/bin/clang++ " + toUTF8(ff.first)) << "\"" << std::endl;
	}
	out << std::setw(4) << j << std::endl;

	return EXIT_SUCCESS;
}

