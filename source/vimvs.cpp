#include "vimvsPCH.h"
#include "Parser.h"
#include "Parameters.h"

/*
Links with information about msbuild
	https://msdn.microsoft.com/en-us/library/ms164311.aspx
Good regular expression builder
	//https://regex101.com/
*/

using namespace cz;
int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	Parameters params(Parameters::Auto);

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

