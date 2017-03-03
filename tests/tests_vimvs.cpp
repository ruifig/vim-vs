#include "testsPCH.h"

/*
Links with information about msbuild
	https://msdn.microsoft.com/en-us/library/ms164311.aspx
Good regular expression builder
	//https://regex101.com/
*/

namespace cz
{
namespace vimvs
{


std::wstring replace(const std::wstring& s, wchar_t from, wchar_t to)
{
	std::wstring res = s;
	for (auto&& ch : res)
	{
		if (ch == from)
			ch = to;
	}
	return res;
}

std::wstring replace(const std::wstring& str, const std::wstring& from, const std::wstring& to)
{
	if (from.empty())
		return L"";
	size_t start_pos = 0;
	auto res = str;
	while ((start_pos = res.find(from, start_pos)) != std::string::npos)
	{
		res.replace(start_pos, from.length(), to);
		start_pos += to.length();  // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
	return res;
}

std::pair<std::wstring, std::wstring> splitFolderAndFile(const std::wstring& str)
{
	auto i = std::find_if(str.rbegin(), str.rend(), [](const wchar_t& ch)
	{
		return ch == '/' || ch == '\\';
	});

	std::pair < std::wstring, std::wstring> res;
	res.first = std::wstring(str.begin(), i.base());
	res.second = std::wstring(i.base(), str.end());
	return res;
}

void ensureTrailingSlash(std::wstring& str)
{
	if (str.size() && !(str[str.size() - 1] == '\\' || str[str.size() - 1] == '/'))
		str += '\\';
}

//
// Converts a string from UTF-8 to UTF-16.
//
std::wstring toUTF16(const std::string& utf8)
{
	if (utf8.empty())
		return std::wstring();

	// Get length (in wchar_t's), so we can reserve the size we need before the
	// actual conversion
	const int length = ::MultiByteToWideChar(CP_UTF8,             // convert from UTF-8
		0,                   // default flags
		utf8.data(),         // source UTF-8 string
		(int)utf8.length(),  // length (in chars) of source UTF-8 string
		NULL,                // unused - no conversion done in this step
		0                    // request size of destination buffer, in wchar_t's
	);
	if (length == 0)
		throw std::exception("Can't get length of UTF-16 string");

	std::wstring utf16;
	utf16.resize(length);

	// Do the actual conversion
	if (!::MultiByteToWideChar(CP_UTF8,             // convert from UTF-8
		0,                   // default flags
		utf8.data(),         // source UTF-8 string
		(int)utf8.length(),  // length (in chars) of source UTF-8 string
		&utf16[0],           // destination buffer
		(int)utf16.length()  // size of destination buffer, in wchar_t's
	))
	{
		throw std::exception("Can't convert string from UTF-8 to UTF-16");
	}

	return utf16;
}

std::string toUTF8(const std::wstring& utf16)
{
	if (utf16.empty())
		return std::string();

	// Get length (in wchar_t's), so we can reserve the size we need before the
	// actual conversion
	const int utf8_length = ::WideCharToMultiByte(CP_UTF8,              // convert to UTF-8
	                                              0,                    // default flags
	                                              utf16.data(),         // source UTF-16 string
	                                              (int)utf16.length(),  // source string length, in wchar_t's,
	                                              NULL,                 // unused - no conversion required in this step
	                                              0,                    // request buffer size
	                                              NULL,
	                                              NULL  // unused
	                                              );

	if (utf8_length == 0)
		throw "Can't get length of UTF-8 string";

	std::string utf8;
	utf8.resize(utf8_length);

	// Do the actual conversion
	if (!::WideCharToMultiByte(CP_UTF8,              // convert to UTF-8
	                           0,                    // default flags
	                           utf16.data(),         // source UTF-16 string
	                           (int)utf16.length(),  // source string length, in wchar_t's,
	                           &utf8[0],             // destination buffer
	                           (int)utf8.length(),   // destination buffer size, in chars
	                           NULL,
	                           NULL  // unused
	                           ))
	{
		throw "Can't convert from UTF-16 to UTF-8";
	}

	return utf8;
}

static bool isSpace(int a)
{
	return a == ' ' || a == '\t' || a == 0xA || a == 0xD;
}

static bool notSpace(int a)
{
	return !isSpace(a);
}

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
std::wstring widen(const std::string& utf8)
{
	if (utf8.empty())
		return std::wstring();

	// Get length (in wchar_t's), so we can reserve the size we need before the
	// actual conversion
	const int length = ::MultiByteToWideChar(CP_UTF8,             // convert from UTF-8
	                                         0,                   // default flags
	                                         utf8.data(),         // source UTF-8 string
	                                         (int)utf8.length(),  // length (in chars) of source UTF-8 string
	                                         NULL,                // unused - no conversion done in this step
	                                         0                    // request size of destination buffer, in wchar_t's
	                                         );
	if (length == 0)
		throw std::exception("Can't get length of UTF-16 string");

	std::wstring utf16;
	utf16.resize(length);

	// Do the actual conversion
	if (!::MultiByteToWideChar(CP_UTF8,             // convert from UTF-8
	                           0,                   // default flags
	                           utf8.data(),         // source UTF-8 string
	                           (int)utf8.length(),  // length (in chars) of source UTF-8 string
	                           &utf16[0],           // destination buffer
	                           (int)utf16.length()  // size of destination buffer, in wchar_t's
	                           ))
	{
		throw std::exception("Can't convert string from UTF-8 to UTF-16");
	}

	return utf16;
}

bool endsWith(const std::wstring& str, const std::wstring& ending)
{
    if (str.length() >= ending.length()) {
        return (0 == str.compare (str.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

bool endsWith(const std::wstring& str, const wchar_t* ending)
{
	const size_t endingLength = wcslen(ending);
    if (str.length() >= endingLength) {
        return (0 == str.compare (str.length() - endingLength, endingLength, ending));
    } else {
        return false;
    }
}

bool beginsWith(const std::wstring& str, const std::wstring& begins)
{
    if (str.length() >= begins.length()) {
        return (0 == str.compare(0, begins.length(), begins));
    } else {
        return false;
    }
}


bool beginsWith(const std::wstring& str, const wchar_t* begins)
{
	const size_t beginsLength = wcslen(begins);
    if (str.length() >= beginsLength) {
        return (0 == str.compare(0, beginsLength, begins));
    } else {
        return false;
    }
}

struct Params
{
	std::vector<std::wstring> defines;
	std::vector<std::wstring> includes;
	std::string readyParams;
	const std::string& getReadyParams()
	{
		if (readyParams.size())
			return readyParams;

		for (auto&& i : defines)
			readyParams += "\"-D" + toUTF8(i) + "\" ";

		for (auto&& i : includes)
			readyParams += "\"-I" + toUTF8(i) + "\" ";

		return readyParams;
	}
};

struct File
{
	std::wstring name;
	std::wstring prjPath;
	std::wstring prjName;
	std::shared_ptr<Params> params;
};

class Database
{
public:
	void addFile(File file)
	{
		m_files[file.name] = std::move(file);
	}

	File* getFile(const std::wstring& filename)
	{
		auto it = m_files.find(filename);
		return it == m_files.end() ? nullptr : &it->second;
	}

	auto& files() const
	{
		return m_files;
	}

private:
	std::unordered_map<std::wstring, File> m_files;
};

class NodeParser;
class Parser
{
public:
	Parser(Database& db);

	void inject(const std::wstring& data);
	void parse(std::wstring line);
private:

	friend class NodeParser;
	Database& m_db;
	std::unordered_map<int, std::shared_ptr<NodeParser>> m_nodes;
	std::shared_ptr<NodeParser> m_currParser;
	bool m_mp = false;
	std::wstring m_line;
};

class NodeParser
{
public:

	explicit NodeParser(Parser& outer, int number, std::wstring prjPath, std::wstring prjName)
		: m_outer(outer)
		, m_number(number)
		, m_prjPath(prjPath)
		, m_prjName(prjName)
	{
	}

	void parseLine(const std::wstring& line)
	{
		if (tryPrjNodeCreation(line))
			return;

		if (tryCompile(line))
			return;
	}

private:

	bool tryPrjNodeCreation(const std::wstring& str)
	{
		// G1 - Project path (without the .vcxproj )
		// G2 - Node number used to build the project
		//                                                                     G1                   G2
		//const wchar_t* re = L"Project \".*\" \\([[:digit:]]+\\) is building \"(.*).vcxproj\" \\(([[:digit:]])\\) on node .*";
		//                                                                     G1      G2                   G3
		const wchar_t* re = L"Project \".*\" \\([[:digit:]]+\\) is building \"(.*)\\\\(.*).vcxproj\" \\(([[:digit:]])\\) on node .*";
		static std::wregex rgx(re, std::regex_constants::egrep | std::regex::optimize);
		std::wsmatch matches;
		if (!std::regex_match(str, matches, rgx))
			return false;
		auto prjPath = matches[1].str();
		auto prjName = matches[2].str();
		int node = std::stoi(matches[3].str());
		wprintf(L"%s , %s, %d\n", prjPath.c_str(), prjName.c_str(), node);
		m_outer.m_nodes[node] = std::make_shared<NodeParser>(m_outer, node, std::move(prjPath), std::move(prjName));
		if (!m_outer.m_mp)
			m_outer.m_currParser = m_outer.m_nodes[node];
		return true;
	}

	bool tryCompile(const std::wstring& line)
	{
		if (trim(line) == L"ClCompile:")
		{
			m_state = State::ClCompile;
			return true;
		}
		if (m_state != State::ClCompile)
			return false;

		// Check if its a call to cl.exe
		{
			const wchar_t* re = L".*\\\\CL\\.exe .*";
			static std::wregex rgx(re, std::regex_constants::egrep | std::regex::optimize);
			std::wsmatch matches;
			if (!std::regex_match(line, matches, rgx))
				return false;
		}


		auto params = std::make_shared<Params>();

		//
		// Extract all defines
		//
		{
			static std::wregex rgx(L"\\/D \"?([[:graph:]]*)\"?");
			for (
				std::wsregex_iterator i = std::wsregex_iterator(line.begin(), line.end(), rgx);
				i != std::wsregex_iterator();
				++i)
			{
				auto&& m = (*i)[1];

				// CMake generated projects can add a macro CMAKE_INTDIR="Debug" to the preprocessor defines, 
				// which msbuild will log as:
				//		/D "CMAKE_INTDIR=\"Debug\"" 
				// The regular expression being used leaves the " at the end (if using "). I'm not an expert at regular
				// expressions, so no idea how to remove that, so removing it manually
				auto s = m.str();
				if (s.back() == '"')
					s.pop_back();
				//params->defines.push_back(replace(s, L"\\\"", L"\""));
				params->defines.push_back(s);
			}
		}

		//
		// Extract all includes
		//
		{
			static std::wregex rgx(L"\\/I\"([[:graph:]]*)\"");
			for (
				std::wsregex_iterator i = std::wsregex_iterator(line.begin(), line.end(), rgx);
				i != std::wsregex_iterator();
				++i)
			{
				auto&& m = (*i)[1];
				params->includes.push_back(replace(m.str(), '\\', '/'));
			}
		}

		//
		// Extract the files to compile. Those are always at the end of the line
		//
		{
			auto e = line.end() - 1;
			while (*e == '"')
			{
				--e;
				auto s = e;
				while (*s != '"') --s;
				File f;
				f.name = std::wstring(s + 1, e+1);
				f.prjPath = m_prjPath;
				f.prjName = m_prjName;
				f.params = params;
				m_outer.m_db.addFile(std::move(f));
				e = s - 1;
				while (isSpace(*e)) --e;
			}
		}

		return true;
	}

	enum class State
	{
		Initial,
		ClCompile,
		Other 
	};

	Parser& m_outer;
	int m_number;
	State m_state = State::Initial;
	std::wstring m_prjPath;
	std::wstring m_prjName;
};

//////////////////////////////////////////////////////////////////////////
Parser::Parser(Database& db)
	: m_db(db)
{
	//m_nodes[0] = std::make_shared<NodeParser>(*this);
}

void Parser::inject(const std::wstring& data)
{
	// New lines format are:
	// Unix		: 0xA
	// Mac		: 0xD
	// Windows	: 0xD 0xA
	for(auto c : data)
	{
		if (c == 0xA || c == 0xD)
		{
			if (m_line.size())
			{
				parse(m_line);
				m_line.clear();
			}
		}
		else
		{
			m_line += c;
		}
	}
}

void Parser::parse(std::wstring line)
{
	if (m_nodes.size()==0)
	{
		static std::wregex rgx(L"[[:space:]]*(1>)?Project \".*\" on node 1.*", std::regex_constants::egrep | std::regex::optimize);
		std::wsmatch matches;
		if (!std::regex_match(line, matches, rgx))
			return;
		m_mp = matches[1].matched;
		m_currParser = std::make_shared<NodeParser>(*this, 1, L"", L"");
		m_nodes[1] = m_currParser;
		return;
	}

	// Detect what node to pass this to
	// Remove the "N>" if present
	{
		static std::wregex rgx(L"[[:space:]]*(([[:digit:]]+)>)?(.*)", std::regex_constants::egrep | std::regex::optimize);
		std::wsmatch matches;
		assert(std::regex_match(line, matches, rgx));
		if (matches[2].matched)
		{
			int n = std::stoi(matches[2].str());
			auto it = m_nodes.find(n);
			if (it == m_nodes.end())
			{
				m_currParser = std::make_shared<NodeParser>(*this, n, L"", L"");
				m_nodes[n] = m_currParser;
			}
			else
			{
				m_currParser = it->second;
			}
		}
		m_currParser->parseLine(matches[3].str());
	}

#if 0
	const wchar_t* prjNode = L"[[:space:]]*(([[:digit:]]+)>)?Project \".*\" \\([[:digit:]]+\\) is building \"(.*).vcxproj\" \\(([[:digit:]])\\) on node .*";
	wprintf(L"RE: %s\n", prjNode);
	static std::wregex rgx(prjNode, std::regex_constants::egrep | std::regex::optimize);

	wprintf(L"  Matching: %s\n", line.c_str());
	std::wsmatch matches;
	if (std::regex_match(line, matches, rgx))
	{
		wprintf(L"    %d matches\n", static_cast<int>(matches.size()));
		for (int i=0; i<matches.size(); i++)
		{
			wprintf(L"      %d:%s:%s\n", i, matches[i].matched ? L"TRUE " : L"FALSE", matches[i].str().c_str());
		}
	}
#endif
}

}
}

SUITE(scratchpad)
{

TEST(1)
{
	using namespace cz::vimvs;
	Database db;
	Parser parser(db);

#if 0
	parser.parse(L"Project \"C:\\Work\\tests.vcxproj.metaproj\" (3) is building \"C:\\Work\\tests.vcxproj\" (8) on node 1 (default targets).");
	parser.parse(L" Project \"C:\\Work\\tests.vcxproj.metaproj\" (3) is building \"C:\\Work\\tests.vcxproj\" (8) on node 1 (default targets).");
	parser.parse(L"	3>Project \"C:\\Work\\tests.vcxproj.metaproj\" (3) is building \"C:\\Work\\tests.vcxproj\" (8) on node 1 (default targets).");
	parser.parse(L"	34>Project \"C:\\Work\\tests.vcxproj.metaproj\" (3) is building \"C:\\Work\\tests.vcxproj\" (8) on node 1 (default targets).");
#endif

	std::ifstream ifs("../../data/vim-vs.msbuild.log");
	CHECK(ifs.is_open());
	auto content = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
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
	commonParams += "\"-isystemC:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include\" ";
	commonParams += "\"-isystemC:/Program Files (x86)/Windows Kits/10/Include/10.0.10150.0/ucrt\" ";
	commonParams += "\"-isystemC:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/atlmfc/include\" ";
	commonParams += "\"-isystemC:/Program Files (x86)/Windows Kits/NETFXSDK/4.6/include/um\" ";
	commonParams += "\"-isystemC:/Program Files (x86)/Windows Kits/8.1/Include/um\" ";
	commonParams += "\"-isystemC:/Program Files (x86)/Windows Kits/8.1/Include/shared\" ";
	commonParams += "\"-isystemC:/Program Files (x86)/Windows Kits/8.1/Include/winrt\" ";

	std::ofstream out("../../compile_commands.json");
	using namespace nlohmann;
	json j;
	//nlohmann::basic_json json;
	for (auto&& f : db.files())
	{
		auto ff = splitFolderAndFile(f.second.name);
		j.push_back({
			{"directory", toUTF8(ff.first)},
			{"command", commonParams + f.second.params->getReadyParams()},
			{"file", toUTF8(f.second.name)}
		});
		//out << "        \"directory\": \"" << json::escape_string(toUTF8(ff.first)) << "\"" << std::endl;
		//out << "        \"command\": \"" << json::escape_string("/usr/bin/clang++ " + toUTF8(ff.first)) << "\"" << std::endl;
	}
	out << std::setw(4) << j << std::endl;

#if 0
	parser.inject(L"Hi");
	parser.inject(L" !\na");
	parser.inject(L"b\n");
	parser.inject(L"");
	parser.inject(L"bb");
	parser.inject(L"\n");
	parser.inject(L"derp");
	printf("Hello world\n!");
#endif
}

}

