#pragma once

#include "Utils.h"
#include "SqLiteWrapper.h"

namespace cz
{

struct SystemIncludes
{
	std::vector<std::string> dirs;
	std::string ready; // #TODO : Remove this
	std::string readyDB;
	const std::string& getIncludes()
	{
		if (ready.size())
			return ready;

		for (auto&& i : dirs)
			ready += "\"-isystem" + i + "\" ";
		return ready;
	}

	const std::string& getIncludesDB()
	{
		if (readyDB.size())
			return readyDB;

		for (auto&& i : dirs)
			readyDB += "-isystem" + i + "|";
		return readyDB;
	}
};

struct Params
{
	std::vector<std::string> defines;
	std::vector<std::string> includes;
	std::string readyParams; // #TODO : Remove this
	std::string readyParamsDB;
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

	const std::string& getReadyParamsDB()
	{
		if (readyParamsDB.size())
			return readyParamsDB;

		for (auto&& i : defines)
			readyParamsDB += "-D" + i + "|";

		for (auto&& i : includes)
			readyParamsDB += "-I" + i + "|";

		return readyParamsDB;
	}
};

struct ParsedFile
{
	std::string name;
	std::string prjName; // Project name (as seen inside VS).
	std::string prjFile; // Full path to the project file
	std::shared_ptr<Params> params;
	std::shared_ptr<SystemIncludes> systemIncludes;
};

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

	void addFile(const ParsedFile& file, bool insertOrReplace);
	SourceFile getFile(const std::string& filename);

private:
	bool getFile(SourceFile& out);
	SqDatabase m_sqdb;
	SqStmt m_sqlGetFile;
	SqStmt m_sqlAddFile;
};

}

