#pragma once

#include "Database.h"

namespace cz
{

class NodeParser;
class Parser
{
public:
	Parser(Database& db, bool updatedb);

	void inject(const std::string& data);
	void parse(std::string line);
private:

	bool tryVimVsBegin(std::string& line);
	bool tryVimVsEnd(std::string& line);

	friend class NodeParser;
	Database& m_db;
	std::unordered_map<int, std::shared_ptr<NodeParser>> m_nodes;
	int m_currNode = 0;
	bool m_mp = false;
	bool m_updatedb = false;
	std::string m_line;
};

class NodeParser
{
public:

	NodeParser(Parser& outer);
	void init(std::string prjName, std::string prjFileName, std::string prjPath, std::shared_ptr<SystemIncludes> systemIncludes);
	void finish();
	bool isFinished() const;
	const std::string& getName() const;
	void parseLine(const std::string& line);
private:
	bool tryCompile(const std::string& line);
	bool tryInclude(const std::string& line);

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
	std::string m_prjPath;
	std::string m_prjFileName;
	std::string m_prjName;
};


}


