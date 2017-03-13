#pragma once

#include "Database.h"

namespace cz
{

class NodeParser;
class Parser
{
public:
	Parser(Database& db);

	void inject(const std::wstring& data);
	void parse(std::wstring line);
private:

	bool tryVimVsBegin(std::wstring& line);
	bool tryVimVsEnd(std::wstring& line);

	friend class NodeParser;
	Database& m_db;
	std::unordered_map<int, std::shared_ptr<NodeParser>> m_nodes;
	int m_currNode = 0;
	bool m_mp = false;
	std::wstring m_line;
};

class NodeParser
{
public:

	NodeParser(Parser& outer);
	void init(std::wstring prjName, std::wstring prjFileName, std::wstring prjPath, std::shared_ptr<SystemIncludes> systemIncludes);
	void finish();
	bool isFinished() const;
	const std::wstring& getName() const;
	void parseLine(const std::wstring& line);
private:
	bool tryCompile(const std::wstring& line);
	bool tryInclude(const std::wstring& line);

	enum class State
	{
		Initial,
		ClCompile,
		Finished
	};

	Parser& m_outer;
	State m_state = State::Initial;
	std::shared_ptr<Params> m_currClCompileParams;
	std::shared_ptr<SystemIncludes> m_systemIncludes;
	std::wstring m_prjPath;
	std::wstring m_prjFileName;
	std::wstring m_prjName;
};


}


