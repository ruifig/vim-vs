#include "testsPCH.h"
#include "Parameters.h"

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

void ensureTrailingSlash(std::wstring& str)
{
	if (str.size() && !(str[str.size()-1] == '\\' || str[str.size()-1] == '/'))
		str += '\\';
}

std::wstring getCWD()
{
	const int bufferLength = MAX_PATH;
	wchar_t buf[bufferLength + 1];
	buf[0] = 0;
	CHECK(GetCurrentDirectoryW(bufferLength, buf) != 0);
	std::wstring res = buf;
	return res + L"\\";
}

// Canonicalizes a path (converts relative to absolute, and converts all '/' characters to '\'
// "root" is used to process relative paths. If it's not specified, it will assume the current working Directory
bool fullPath(std::wstring& dst, const std::wstring& path, std::wstring root)
{
	wchar_t fullpathbuf[MAX_PATH+1];
	wchar_t srcfullpath[MAX_PATH+1];
	if (root.empty())
		root = getCWD();
	ensureTrailingSlash(root);

	std::wstring tmp = PathIsRelativeW(path.c_str()) ? root + path : path;
	wcscpy(srcfullpath, tmp.c_str());
	wchar_t* d = srcfullpath;
	wchar_t* s = srcfullpath;
	while(*s)
	{
		if (*s == '/')
			*s = '\\';
		*d++= *s;

		// Skip any repeated separator
		if (*s == '\\')
		{
			s++;
			while (*s && (*s == '\\' || *s == '/'))
				s++;
		}
		else
		{
			s++;
		}
	}
	*d = 0;

	bool res = PathCanonicalizeW(fullpathbuf, srcfullpath) ? true : false;
	if (res)
		dst = fullpathbuf;
	return res;
}

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

struct SystemIncludes
{
	std::vector<std::wstring> dirs;
	std::string ready;
	const std::string& getIncludes()
	{
		if (ready.size())
			return ready;

		for (auto&& i : dirs)
			ready += "\"-isystem" + toUTF8(i) + "\" ";
		return ready;
	}
};


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
	std::wstring prjName;
	std::shared_ptr<Params> params;
	std::shared_ptr<SystemIncludes> systemIncludes;
};

class Database
{
public:
	void addFile(File file)
	{
		wprintf(L"%s: %s\n", file.prjName.c_str(), file.name.c_str());
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

	explicit NodeParser(Parser& outer, int number, std::wstring prjPath, std::wstring prjFileName, std::wstring prjName)
		: m_outer(outer)
		, m_number(number)
		, m_prjPath(prjPath)
		, m_prjFileName(prjFileName)
		, m_prjName(prjName)
	{
	}

	void parseLine(const std::wstring& line)
	{
		if (tryPrjNodeCreation(line))
			return;

		if (tryPrjIncludePath(line))
			return;

		if (tryCompile(line))
			return;

		if (tryInclude(line))
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
		ensureTrailingSlash(prjPath);
		auto prjFileName = matches[2].str();
		int node = std::stoi(matches[3].str());
		wprintf(L"%s , %s, %d\n", prjPath.c_str(), prjFileName.c_str(), node);
		m_outer.m_nodes[node] = std::make_shared<NodeParser>(m_outer, node, std::move(prjPath), prjFileName, prjFileName);
		if (!m_outer.m_mp)
			m_outer.m_currParser = m_outer.m_nodes[node];

		return true;
	}

	bool tryPrjIncludePath(const std::wstring& line)
	{
		if (trim(line) == L"PreBuildEvent:")
		{
			m_state = State::PreBuildEvent;
			return true;
		}

		if (m_state != State::PreBuildEvent)
			return false;

		//static std::wregex rgx(L"[[:space:]]*rem IncludePath=(.+)", std::regex_constants::egrep | std::regex::optimize);
		static std::wregex rgx(L"[[:space:]]*rem vim-vs: ProjectName=\"(.+)\", ProjectFileName=\"(.+)\", IncludePath=(.+)", std::regex_constants::egrep | std::regex::optimize);
		std::wsmatch matches;
		if (!std::regex_match(line, matches, rgx))
			return false;

		assert(!m_systemIncludes);
		m_systemIncludes = std::make_shared<SystemIncludes>();

		auto prjName = matches[1].str();
		auto prjFileName = matches[2].str();
		auto dirs = matches[3].str();

		size_t s = 0;
		size_t e = 0;
		while (e!=std::wstring::npos)
		{
			e = dirs.find(';', s);
			std::wstring str;
			if (e == std::wstring::npos)
				str = dirs.substr(s);
			else
			{
				str = dirs.substr(s, e - s);
				e++;
			}
			str = trim(str);
			if (str.size())
				m_systemIncludes->dirs.push_back(replace(trim(str), '\\', '/'));
			s = e;
		}

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

		m_currClCompileParams = std::make_shared<Params>();

		//
		// Extract all defines
		//
		{
			static std::wregex rgx(L"\\/D \"?([[:graph:]]*)\"?", std::regex::optimize);
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
				m_currClCompileParams->defines.push_back(s);
			}
		}

		//
		// Extract all includes
		//
		{
			static std::wregex rgx(L"\\/I\"([[:graph:]]*)\"", std::regex::optimize);
			for (
				std::wsregex_iterator i = std::wsregex_iterator(line.begin(), line.end(), rgx);
				i != std::wsregex_iterator();
				++i)
			{
				auto&& m = (*i)[1];
				m_currClCompileParams->includes.push_back(replace(m.str(), '\\', '/'));
			}
		}

		//
		// Extract the files to compile. Those are always at the end of the line
		// We extract them from the end until we find something is is not a file.
		//
		{
			std::vector<std::wstring> tokens;
			// e : points to the last char in the token
			auto e = line.end() - 1;
			while (true)
			{
				while (*e == ' ') e--; // remove spaces
				bool quoted = *e == '"' ? true : false;
				if (quoted) e--; // Remove the "

				// s : Points to one character behind the token start
				std::wstring::const_iterator s;
				if (quoted)
				{
					s = line.begin() + line.rfind('"', e - line.begin());
					// msbuild outputs some parameters such as:  /Fd"UnitTest++.dir\Debug\UnitTest++.pdb"
					// , so  keep processing until we find the space, so we take all that as 1 token
					if (*(s - 1) != ' ')
					{
						s--;
						while (*s!=' ') s--;
					}
				}
				else
				{
					s = line.begin() + line.rfind(' ', e - line.begin());
				}

				auto token = std::wstring(s+1, e+1);

				// Exit conditions are:
				// - Token starts with /
				//		- Additional, if token is a /D, then also drop the previously processor token (which is the macro)
				wprintf(L"*%s*\n", token.c_str());
				if (*token.begin() == '/')
				{
					if (token == L"/D")
						tokens.pop_back();
					break;
				}
				tokens.push_back(std::move(token));
				e = s-1;
			}

			for (auto it = tokens.rbegin(); it != tokens.rend(); ++it)
			{
				File f;
				CHECK(fullPath(f.name, *it, m_prjPath));
				f.prjName = m_prjName;
				f.systemIncludes = m_systemIncludes;
				f.params = m_currClCompileParams;
				m_outer.m_db.addFile(std::move(f));
			}
		}

#if 0
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
				f.systemIncludes = m_systemIncludes;
				f.params = m_currClCompileParams;
				m_outer.m_db.addFile(std::move(f));
				e = s - 1;
				while (isSpace(*e)) --e;
			}
		}
#endif

		return true;
	}

	bool tryInclude(const std::wstring& line)
	{
		if (m_state != State::ClCompile)
			return false;

		static std::wregex rgx(L"Note: including file:[[:space:]]*(.*)", std::regex_constants::egrep | std::regex::optimize);
		std::wsmatch matches;
		if (!std::regex_match(line, matches, rgx))
			return false;

		// At this point, we should have compile parameters set, since we are compiling a file
		assert(m_currClCompileParams);
		auto fname = matches[1].str();

		// Check if this file was already in the database
		if (m_outer.m_db.getFile(fname) != nullptr)
			return true;

		File f;
		f.name = fname;
		f.prjName = m_prjName;
		f.systemIncludes = m_systemIncludes;
		f.params = m_currClCompileParams;
		m_outer.m_db.addFile(std::move(f));
		return true;
	}

	enum class State
	{
		Initial,
		PreBuildEvent,
		ClCompile,
		Other 
	};

	Parser& m_outer;
	int m_number;
	State m_state = State::Initial;
	std::shared_ptr<Params> m_currClCompileParams;
	std::shared_ptr<SystemIncludes> m_systemIncludes;
	std::wstring m_prjPath;
	std::wstring m_prjFileName;
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
	size_t line = 1;
	for(auto c : data)
	{
		if (c == 0xA || c == 0xD)
		{
			if (m_line.size())
			{
				parse(m_line);
				m_line.clear();
			}
			line++;
		}
		else
		{
			m_line += c;
		}
	}
}

void Parser::parse(std::wstring line)
{
	//wprintf(L"%s\n", line.c_str());
	if (m_nodes.size()==0)
	{
		static std::wregex rgx(L"[[:space:]]*(1>)?Project \".*\" on node 1.*", std::regex_constants::egrep | std::regex::optimize);
		std::wsmatch matches;
		if (!std::regex_match(line, matches, rgx))
			return;
		m_mp = matches[1].matched;
		m_currParser = std::make_shared<NodeParser>(*this, 1, L"", L"", L"");
		m_nodes[1] = m_currParser;
		return;
	}

	// Detect what node to pass this to
	// Remove the "N>" if present
	{
		static std::wregex rgx(L"[[:space:]]*(([[:digit:]]+)>)?(.*)", std::regex_constants::egrep | std::regex::optimize);
		std::wsmatch matches;
		if (!std::regex_match(line, matches, rgx))
		{
			assert(0);
		}

		if (matches[2].matched)
		{
			int n = std::stoi(matches[2].str());
			auto it = m_nodes.find(n);
			if (it == m_nodes.end())
			{
				m_currParser = std::make_shared<NodeParser>(*this, n, L"", L"", L"");
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
	Parameters params(Parameters::Auto);

#if 0
	parser.parse(L"Project \"C:\\Work\\tests.vcxproj.metaproj\" (3) is building \"C:\\Work\\tests.vcxproj\" (8) on node 1 (default targets).");
	parser.parse(L" Project \"C:\\Work\\tests.vcxproj.metaproj\" (3) is building \"C:\\Work\\tests.vcxproj\" (8) on node 1 (default targets).");
	parser.parse(L"	3>Project \"C:\\Work\\tests.vcxproj.metaproj\" (3) is building \"C:\\Work\\tests.vcxproj\" (8) on node 1 (default targets).");
	parser.parse(L"	34>Project \"C:\\Work\\tests.vcxproj.metaproj\" (3) is building \"C:\\Work\\tests.vcxproj\" (8) on node 1 (default targets).");
#endif

	

	//std::ifstream ifs("../../data/vim-vs.msbuild.log");
	std::wstring srcfile = params.Get(L"in");
	if (srcfile.size()==0)
		srcfile = L"../../data/showincludes_2.log";
	std::wstring dstfile = params.Get(L"out");
	if (dstfile.size()==0)
		dstfile = L"../../compile_commands.json";
	std::ifstream ifs(srcfile);
	CHECK(ifs.is_open());
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
#if 0
	commonParams += "\"-isystemC:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include\" ";
	commonParams += "\"-isystemC:/Program Files (x86)/Windows Kits/10/Include/10.0.10150.0/ucrt\" ";
	commonParams += "\"-isystemC:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/atlmfc/include\" ";
	commonParams += "\"-isystemC:/Program Files (x86)/Windows Kits/NETFXSDK/4.6/include/um\" ";
	commonParams += "\"-isystemC:/Program Files (x86)/Windows Kits/8.1/Include/um\" ";
	commonParams += "\"-isystemC:/Program Files (x86)/Windows Kits/8.1/Include/shared\" ";
	commonParams += "\"-isystemC:/Program Files (x86)/Windows Kits/8.1/Include/winrt\" ";
#endif

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

