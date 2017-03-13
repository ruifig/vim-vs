#pragma once

#include "Utils.h"

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

}

