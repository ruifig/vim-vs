#pragma once

#include "Database.h"

namespace cz
{

struct Error
{
	std::string file;
	int line = 0;
	int col = 0;
	std::string type; // error/warning
	std::string code;
	std::string msg;
};

class NodeParser;
class Parser
{
public:
	Parser(Database& db, bool updatedb, bool parseErrors);

	void inject(const std::string& data);
	
	const std::vector<Error>& getErrors() const
	{
		return m_errors;
	}
private:

	bool parse(std::string line);
	bool tryVimVsBegin(std::string& line);
	bool tryVimVsEnd(std::string& line);
	bool tryError(const std::string& line);

	friend class NodeParser;
	Database& m_db;
	std::unordered_map<int, std::shared_ptr<NodeParser>> m_nodes;
	int m_currNode = 0;
	bool m_mp = false;
	bool m_updatedb = false;
	bool m_parseErrors = false;
	std::vector<Error> m_errors;
	std::string m_line;
};

class NodeParser
{
public:

	NodeParser(Parser& outer);
	void init(std::string prjName, std::string prjFile, std::shared_ptr<SystemIncludes> systemIncludes);
	void finish();
	bool isFinished() const;
	const std::string& getName() const;
	bool parseLine(const std::string& line);
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
	std::string m_prjFile; // Full path to the project file
	std::string m_prjDir;
	std::string m_prjName;
};


}


