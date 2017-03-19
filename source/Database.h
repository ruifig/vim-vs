#pragma once

#include "Utils.h"
#include "SqLiteWrapper.h"

namespace cz
{

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

struct SourceFile
{
	uint64_t id = 0;
	std::wstring fullpath;
	std::wstring name;
	std::wstring projectName;
	std::wstring configuration;
	std::wstring defines;
	std::wstring includes;
};

class Database
{
public:
	Database();
	bool open(const std::wstring& dbfname);
	void addFile(File file);
	File* getFile(const std::wstring& filename);
	auto& files() const
	{
		return m_files;
	}
private:

	bool getSourceFile(SourceFile& out);

	SqDatabase m_sqdb;
	SqStmt m_sqlGetFile;
	SqStmt m_sqlAddFile;

	std::unordered_map<std::wstring, File> m_files;
};

}

