#pragma once

#include "Utils.h"
#include "SqLiteWrapper.h"

namespace cz
{

struct SystemIncludes
{
	std::vector<std::string> dirs;
	std::string ready;
	const std::string& getIncludes()
	{
		if (ready.size())
			return ready;

		for (auto&& i : dirs)
			ready += "\"-isystem" + i + "\" ";
		return ready;
	}
};

struct Params
{
	std::vector<std::string> defines;
	std::vector<std::string> includes;
	std::string readyParams;
	const std::string& getReadyParams()
	{
		if (readyParams.size())
			return readyParams;

		for (auto&& i : defines)
			readyParams += "\"-D" + i + "\" ";

		for (auto&& i : includes)
			readyParams += "\"-I" + i + "\" ";

		return readyParams;
	}
};

struct File
{
	std::string name;
	std::string prjName;
	std::shared_ptr<Params> params;
	std::shared_ptr<SystemIncludes> systemIncludes;
};

struct SourceFile
{
	uint64_t id = 0;
	std::string fullpath;
	std::string name;
	std::string projectName;
	std::string configuration;
	std::string defines;
	std::string includes;
};

class Database
{
public:
	Database();
	bool open(const std::string& dbfname);
	void addFile(File file);
	File* getFile(const std::string& filename);
	auto& files() const
	{
		return m_files;
	}
private:

	bool getSourceFile(SourceFile& out);

	SqDatabase m_sqdb;
	SqStmt m_sqlGetFile;
	SqStmt m_sqlAddFile;

	std::unordered_map<std::string, File> m_files;
};

}

