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


bool beginsWith(const std::wstring& str, const std::wstring& begins)
{
	const size_t beginsLength = wcslen(begin);
    if (str.length() >= begins.length()) {
        return (0 == str.compare(0, beginsLength, begins));
    } else {
        return false;
    }
}

bool contains(const std::wstring)

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

class NodeParser
{
public:
	explicit NodeParser(Database& db, std::wstring prjName=L"") : m_db(db)
	{
	}

	void parseLine(std::wstring line)
	{
		m_str = std::move(line);
		m_it = m_str.begin();
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

	Database& m_db;
	State m_state = State::Initial;
	std::wstring m_str;
	std::wstring m_prjName;
	std::wstring::iterator m_it;
};


class Parser
{
public:
	explicit Parser(Database& db) : m_db(db)
	{
	}

	void parse(std::wstring line)
	{
		if (!m_currNode)
		{
			if (beginsWith(line, L"Project ") && )
			if (beginsWith(line, L"Build started "))
				


		}


		if (m_currNode)
		{
		}
	}

private:
	Database& m_db;
	NodeParser* m_currNode = nullptr;
	std::unordered_map<int, NodeParser> m_nodes;
};

}
}


SUITE(scratchpad)
{

TEST(1)
{
	printf("Hello world\n!");
}

}

