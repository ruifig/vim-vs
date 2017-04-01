#pragma once

#include "Utils.h"
#include "SqLiteWrapper.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <set>
#include "BuildGraph.h"

namespace cz
{

inline std::string joinParamsDB(const char* prefix, const std::vector<std::string>& v)
{
	std::string res;
	for (auto&& i : v)
		res += prefix + i + "|";
	return res;
}

inline std::string joinDefines(const std::vector<std::string>& v)
{
	return joinParamsDB("-D", v);
}

inline std::string joinSystemIncludes(const std::vector<std::string>& v)
{
	return joinParamsDB("-isystem", v);
}

inline std::string joinUserIncs(const std::vector<std::string>& v)
{
	return joinParamsDB("-I", v);
}

struct SourceFile
{
	uint64_t id = 0;
	std::string fullpath;
	std::string name;
	std::string prjName;
	std::string prjFile;
	std::string configuration;
	std::string defines;
	std::string includes;
};

class Database
{
public:
	Database();
	bool open(const std::string& dbfname);

	void addFile(
		const std::string& fullpath,
		const std::string& prjName, const std::string& prjFile,
		const std::string& defines,
		const std::string& includes,
		bool insertOrReplace);
	SourceFile getFile(const std::string& filename);
	std::vector<SourceFile> getWithBasename(const std::string& filename);
private:
	bool getFile(SourceFile& out);
	SqDatabase m_sqdb;
	SqStmt m_sqlGetFile;
	SqStmt m_sqlGetWithBasename;
	SqStmt m_sqlAddFile;
	std::set<uint64_t> m_inserted;
};

}

