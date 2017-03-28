#include "vimvsPCH.h"
#include "Parser.h"

namespace cz
{

//////////////////////////////////////////////////////////////////////////
//		Parser
//////////////////////////////////////////////////////////////////////////
Parser::Parser(Database& db, bool updatedb, bool parseErrors)
	: m_db(db)
	, m_updatedb(updatedb)
	, m_parseErrors(parseErrors)
{
}

void Parser::inject(const std::string& data)
{
	// New lines format are:
	// Unix		: 0xA
	// Mac		: 0xD
	// Windows	: 0xD 0xA
	size_t line = 1;
	for(auto c : data)
	{
		if (c == 0xA || c == 0xD)
		{
			if (m_line.size())
			{
				bool consumed = false;
				if (m_updatedb)
				{
					printf("%s\n", m_line.c_str());
					consumed = parse(m_line);
				}
				else
					printf("%s\n", m_line.c_str());

				if (!consumed && m_parseErrors)
					tryError(m_line);
				m_line.clear();
			}
			line++;
		}
		else
		{
			m_line += c;
		}
	}
}

bool Parser::tryVimVsBegin(std::string& line)
{
	static std::regex rgx(
		"[[:space:]]*rem vim-vs-begin: ProjectName=\"(.+)\", ProjectPath=\"(.+)\", IncludePath=(.+)",
		std::regex_constants::egrep | std::regex::optimize);
	std::smatch matches;
	if (!std::regex_match(line, matches, rgx))
		return false;

	auto systemIncludes = std::make_shared<SystemIncludes>();

	auto projectName = matches[1].str();
	auto projectPath = matches[2].str();
	auto includePath = matches[3].str();

	size_t s = 0;
	size_t e = 0;
	while (e != std::string::npos)
	{
		e = includePath.find(';', s);
		std::string str;
		if (e == std::string::npos)
			str = includePath.substr(s);
		else
		{
			str = includePath.substr(s, e - s);
			e++;
		}
		str = trim(str);
		if (str.size())
			systemIncludes->dirs.push_back(replace(trim(str), '\\', '/'));
		s = e;
	}

	if (!m_mp)
		m_currNode++;
	auto node = std::make_shared<NodeParser>(*this);
	m_nodes[m_currNode] = node;
	node->init(projectName, projectPath, systemIncludes);

	return true;
}

bool Parser::tryVimVsEnd(std::string& line)
{
	static std::regex rgx(
		"[[:space:]]*rem vim-vs-end: ProjectName=\"(.+)\"",
		std::regex_constants::egrep | std::regex::optimize);
	std::smatch matches;
	if (!std::regex_match(line, matches, rgx))
		return false;

	auto projectName = matches[1].str();
	auto it = m_nodes.find(m_currNode);
	CZ_CHECK(it != m_nodes.end() && it->second->getName() == projectName);

	it->second->finish();

	return true;
}

bool Parser::tryError(const std::string& line)
{
	static std::regex rgx(
		//                1          <2:File > |3:|  4: line   |       |    5        | |       6         |  |7 |
		"[[:space:]]*([[:digit:]]*>)?([^\\(]*)(\\(([[:digit:]]+)\\))?: (error|warning) ([A-Z][[:digit:]]*): (.+)",
		std::regex::optimize);
	static std::regex rgx2(
		//1                         2             3                4
		"(.*) : Command line (error|warning) ([A-Z][[:digit:]]*): (.*)",
		std::regex::optimize
	);

	std::smatch matches;
	Error err;
	if (std::regex_match(line, matches, rgx))
	{
		err.file = matches[2].str();
		err.line = std::stoi(matches[4].str());
		err.type = matches[5].str();
		err.code = matches[6].str();
		err.msg = matches[7].str();
	}
	else if (std::regex_match(line, matches, rgx2))
	{
		err.type = matches[2].str();
		err.code = matches[3].str();
		err.msg = matches[4].str();
	}
	else
	{
		return false;
	}

	// Look for repeated errors/warnings
	for (auto&& e : m_errors)
	{
		if (e.line == err.line && e.file == err.file && e.code == err.code)
			return true;
	}
	m_errors.push_back(std::move(err));


	return true;
}

bool Parser::parse(std::string line)
{
	if (m_currNode==0)
	{
		static std::regex rgx("[[:space:]]*(1>)?Project \".*\" on node 1.*", std::regex_constants::egrep | std::regex::optimize);
		std::smatch matches;
		if (!std::regex_match(line, matches, rgx))
			return false;
		m_mp = matches[1].matched;
		m_currNode = 1;
		return true;
	}

	// Detect what node to pass this to
	// Remove the "N>" if present
	static std::regex rgx("[[:space:]]*(([[:digit:]]+)>)?(.*)", std::regex_constants::egrep | std::regex::optimize);
	std::smatch matches;
	CZ_CHECK(std::regex_match(line, matches, rgx));
	if (matches[2].matched) // The node number
	{
		auto n = std::stoi(matches[2].str()); // # TODO : Remove this
		m_currNode = std::stoi(matches[2].str());
	}

	line = matches[3].str();
	if (tryVimVsBegin(line))
		return true;
	if (tryVimVsEnd(line))
		return true;

	auto it = m_nodes.find(m_currNode);
	if (it != m_nodes.end() && !it->second->isFinished())
	{
		return it->second->parseLine(line);
	}

	return false;

}

//////////////////////////////////////////////////////////////////////////
//		NodeParser
//////////////////////////////////////////////////////////////////////////
NodeParser::NodeParser(Parser& outer) : m_outer(outer)
{
}

void NodeParser::init(std::string prjName, std::string prjFile, std::shared_ptr<SystemIncludes> systemIncludes)
{
	auto s = splitFolderAndFile(prjFile);
	m_prjName = prjName;
	m_prjDir = s.first;
	m_prjFile = prjFile;
	m_systemIncludes = systemIncludes;
}

void NodeParser::finish()
{
	m_state = State::Finished;
}

bool NodeParser::isFinished() const
{
	return m_state == State::Finished;
}

const std::string& NodeParser::getName() const
{
	return m_prjName;
}

bool NodeParser::parseLine(const std::string& line)
{
	if (tryCompile(line))
		return true;

	if (tryInclude(line))
		return true;

	return false;
}

bool NodeParser::tryCompile(const std::string& line)
{
	if (trim(line) == "ClCompile:")
	{
		m_state = State::ClCompile;
		return true;
	}
	if (m_state != State::ClCompile)
		return false;

	// Check if its a call to cl.exe
	{
		const char* re = ".*\\\\CL\\.exe .*";
		static std::regex rgx(re, std::regex_constants::egrep | std::regex::optimize);
		std::smatch matches;
		if (!std::regex_match(line, matches, rgx))
			return false;
	}

	m_currClCompileParams = std::make_shared<Params>();

	//
	// Extract all defines
	//
	{
		static std::regex rgx("\\/D \"?([[:graph:]]*)\"?", std::regex::optimize);
		for (
			std::sregex_iterator i = std::sregex_iterator(line.begin(), line.end(), rgx);
			i != std::sregex_iterator();
			++i)
		{
			auto&& m = (*i)[1];

			// CMake generated projects can add a macro CMAKE_INTDIR="Debug" to the preprocessor defines, 
			// which msbuild will log as:
			//		/D "CMAKE_INTDIR=\"Debug\"" 
			// The regular expression being used leaves the " at the end (if using "). I'm not an expert at regular
			// expressions, so no idea how to remove that, so removing it manually
			auto s = m.str();
			if (s.back() == '"')
				s.pop_back();
			s = replace(s, "\\\"", "\"");
			m_currClCompileParams->defines.push_back(s);
		}
	}

	//
	// Extract all includes
	//
	{
		printf("%s\n", line.c_str());
		static std::regex rgx("[[:space:]]\\/I\"([^\"]+)\"", std::regex::optimize);
		for (
			std::sregex_iterator i = std::sregex_iterator(line.begin(), line.end(), rgx);
			i != std::sregex_iterator();
			++i)
		{
			auto&& m = (*i)[1];
			m_currClCompileParams->includes.push_back(replace(m.str(), '\\', '/'));
		}
	}
	{
		static std::regex rgx("[[:space:]]\\/I([^\\s,^\"]+)", std::regex::optimize);
		for (
			std::sregex_iterator i = std::sregex_iterator(line.begin(), line.end(), rgx);
			i != std::sregex_iterator();
			++i)
		{
			auto&& m = (*i)[1];
			m_currClCompileParams->includes.push_back(replace(m.str(), '\\', '/'));
		}
	}

	//
	// Extract the files to compile. Those are always at the end of the line
	// We extract them from the end until we find something is is not a file.
	//
	std::vector<std::string> tokens;
	{
		// e : points to the last char in the token
		auto e = line.end() - 1;
		while (true)
		{
			while (*e == ' ') e--; // remove spaces
			bool quoted = *e == '"' ? true : false;
			if (quoted) e--; // Remove the "

			// s : Points to one character behind the token start
			std::string::const_iterator s;
			if (quoted)
			{
				s = line.begin() + line.rfind('"', e - line.begin());
				// msbuild outputs some parameters such as:  /Fd"UnitTest++.dir\Debug\UnitTest++.pdb"
				// , so  keep processing until we find the space, so we take all that as 1 token
				if (*(s - 1) != ' ')
				{
					s--;
					while (*s != ' ') s--;
				}
			}
			else
			{
				s = line.begin() + line.rfind(' ', e - line.begin());
			}

			auto token = std::string(s + 1, e + 1);

			// Exit conditions are:
			// - Token starts with /
			//		- Additional, if token is a /D, then also drop the previously processor token (which is the macro)
			printf("*%s*\n", token.c_str());
			if (*token.begin() == '/')
			{
				if (token == "/D")
					tokens.pop_back();
				break;
			}
			tokens.push_back(std::move(token));
			e = s - 1;
		}
	}

	if (m_outer.m_updatedb)
	{
		for (auto it = tokens.rbegin(); it != tokens.rend(); ++it)
		{
			ParsedFile f;
			CZ_CHECK(fullPath(f.name, *it, m_prjDir));
			f.prjName = m_prjName;
			f.prjFile = m_prjFile;
			f.systemIncludes = m_systemIncludes;
			f.params = m_currClCompileParams;
			m_outer.m_db.addFile(std::move(f), true);
		}
	}

	return true;
}

bool NodeParser::tryInclude(const std::string& line)
{
	if (m_state != State::ClCompile)
		return false;

	static std::regex rgx("Note: including file:[[:space:]]*(.*)", std::regex_constants::egrep | std::regex::optimize);
	std::smatch matches;
	if (!std::regex_match(line, matches, rgx))
		return false;

	// At this point, we should have compile parameters set, since we are compiling a file
	assert(m_currClCompileParams);
	auto fname = matches[1].str();

	if (!m_outer.m_updatedb)
		return true;

	// Check if this file was already in the database
	if (m_outer.m_db.getFile(fname).id)
		return true;

	ParsedFile f;
	f.name = fname;
	f.prjName = m_prjName;
	f.prjFile = m_prjFile;
	f.systemIncludes = m_systemIncludes;
	f.params = m_currClCompileParams;
	m_outer.m_db.addFile(std::move(f), true);
	return true;
}

} // namespace cz

