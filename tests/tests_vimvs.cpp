#include "testsPCH.h"

/*
Links with information about msbuild
	https://msdn.microsoft.com/en-us/library/ms164311.aspx
*/

namespace cz
{
namespace vimvs
{

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
	void parse(std::string line);
private:

	friend class NodeParser;
	Database& m_db;
	std::unordered_map<int, std::shared_ptr<NodeParser>> m_nodes;
};

class NodeParser
{
public:

	explicit NodeParser(Parser& outer, std::wstring prjName=L"") : m_outer(outer)
	{
	}

	void parseLine(std::wstring line)
	{
	}

private:

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

	Parser m_outer;
	State m_state = State::Initial;
	std::wstring m_prjName;
};

//////////////////////////////////////////////////////////////////////////
Parser::Parser(Database& db) : m_db(db)
{
	m_nodes[0] = std::make_shared<NodeParser>(*this);
}

void Parser::parse(std::string line)
{
	// Original: \s*((\d*)>)?Project \".+\" \(\d\) is building \"(.+)\.vcxproj\" \((\d)\) on node \d
	//https://regex101.com/
	const char* prjNode = "[[:space:]]*(([[:digit:]]+)>)?Project \".*\" \\([[:digit:]]+\\) is building \"(.*).vcxproj\" \\(([[:digit:]])\\) on node .*";
	printf("RE: %s\n", prjNode);
	std::regex rgx(prjNode, std::regex_constants::egrep | std::regex::optimize);

	printf("  Matching: %s\n", line.c_str());
	std::smatch matches;
	if (std::regex_match(line, matches, rgx))
	{
		printf("    %d matches\n", static_cast<int>(matches.size()));
		for (int i=0; i<matches.size(); i++)
		{
			printf("      %d:%s:%s\n", i, matches[i].matched ? "TRUE " : "FALSE", matches[i].str().c_str());
		}
	}
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

	parser.parse("Project \"C:\\Work\\tests.vcxproj.metaproj\" (3) is building \"C:\\Work\\tests.vcxproj\" (8) on node 1 (default targets).");
	parser.parse(" Project \"C:\\Work\\tests.vcxproj.metaproj\" (3) is building \"C:\\Work\\tests.vcxproj\" (8) on node 1 (default targets).");
	parser.parse("	3>Project \"C:\\Work\\tests.vcxproj.metaproj\" (3) is building \"C:\\Work\\tests.vcxproj\" (8) on node 1 (default targets).");
	parser.parse("	34>Project \"C:\\Work\\tests.vcxproj.metaproj\" (3) is building \"C:\\Work\\tests.vcxproj\" (8) on node 1 (default targets).");

	printf("Hello world\n!");
}

}

