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

static bool isSpace(int a)
{
	return a == ' ' || a == '\t' || a == 0xA || a == 0xD;
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

struct FileInfo
{
	std::wstring filename;
	std::vector<std::pair<std::wstring, std::wstring>> defines;
	std::vector<std::wstring> includepaths;
};

class Database
{
public:
	void addFile(std::wstring filename, FileInfo info)
	{
		m_files[filename] = std::move(info);
	}

	FileInfo* getFile(const std::wstring& filename)
	{
		auto it = m_files.find(filename);
		return it == m_files.end() ? nullptr : &it->second;
	}
private:
	std::unordered_map<std::wstring, FileInfo> m_files;
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

	explicit NodeParser(Parser& outer, int number, std::wstring prjName=L"")
		: m_outer(outer)
		, m_number(number)
		, m_prjName(prjName)
	{
	}

	void parseLine(std::wstring line)
	{
		if (tryPrjNodeCreation(line))
			return;
	}

private:

	bool tryPrjNodeCreation(const std::wstring str)
	{
		// G1 - Project path (without the .vcxproj )
		// G2 - Node number used to build the project
		//                                                                     G1                   G2
		const wchar_t* re = L"Project \".*\" \\([[:digit:]]+\\) is building \"(.*).vcxproj\" \\(([[:digit:]])\\) on node .*";
		static std::wregex rgx(re, std::regex_constants::egrep | std::regex::optimize);
		std::wsmatch matches;
		if (!std::regex_match(str, matches, rgx))
			return false;
		wprintf(L"PRJ %s\n", matches[1].str().c_str());
		int n = std::stoi(matches[2].str());
		m_outer.m_nodes[n] = std::make_shared<NodeParser>(m_outer, n, matches[1].str());
		if (!m_outer.m_mp)
			m_outer.m_currParser = m_outer.m_nodes[n];
		return true;
	}

	std::vector<FileInfo> parseCompileString(std::wstring str)
	{
		std::vector<FileInfo> res;
		return res;
	}

	enum class State
	{
		Initial,
		ClCompile,
		Other, 
		FinalizeBuildStatus
	};

	Parser& m_outer;
	int m_number;
	State m_state = State::Initial;
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
		m_currParser = std::make_shared<NodeParser>(*this, 1);
		m_nodes[1] = m_currParser;
		return;
	}

	// Detect what node to pass this to
	// Remove the "N>" if 
	{
		//                                         G2            G3
		static std::wregex rgx(L"[[:space:]]*(([[:digit:]]+)>)?(.*)", std::regex_constants::egrep | std::regex::optimize);
		std::wsmatch matches;
		assert(std::regex_match(line, matches, rgx));
		if (matches[2].matched)
		{
			int n = std::stoi(matches[2].str());
			auto it = m_nodes.find(n);
			if (it == m_nodes.end())
			{
				m_currParser = std::make_shared<NodeParser>(*this, n);
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

	std::ifstream ifs("../../data/test2.log");
	CHECK(ifs.is_open());
	auto content = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
	parser.inject(widen(content));

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

