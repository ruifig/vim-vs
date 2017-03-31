#include "vimvsPCH.h"
#include "Parser.h"

namespace cz
{

static const bool gAsync = false;

//////////////////////////////////////////////////////////////////////////
//		Parser
//////////////////////////////////////////////////////////////////////////
Parser::Parser(Database& db, bool updatedb, bool parseErrors, bool fastParser)
	: m_db(db)
	, m_updatedb(updatedb)
	, m_parseErrors(parseErrors)
	, m_fastParser(fastParser)
{
	m_clNameRgx = std::regex(
		formatString(".*\\\\%s\\.exe .*", fastParser ? VIMVS_FAST_PARSER_CL : "CL"),
		std::regex_constants::egrep | std::regex::optimize);
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

void Parser::finishWork()
{
	if (m_fastParser)
	{
		m_graph.finishWork();
		m_graph.iterate([&](const std::shared_ptr<buildgraph::Node>& n)
		{
			if (n->getType() != buildgraph::Node::Type::Header)
				return;
			auto incDirs = n->getIncludeDirs();
			m_db.addFile(n->getName(), "", "",
				joinDefines(n->getDefines()),
				joinUserIncs(incDirs->getUserIncs()) + joinSystemIncludes(incDirs->getSystemIncs()),
				true);

		});
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

	std::vector<std::string> systemIncs;
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
		{
			CZ_CHECK(fullPath(str, str, ""));
			systemIncs.push_back(str);
		}
		s = e;
	}

	if (!m_mp)
		m_currNode++;
	auto node = std::make_shared<NodeParser>(*this);
	m_nodes[m_currNode] = node;
	node->init(projectName, projectPath, std::move(systemIncs));

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
		//                1         <2:File >|3:|  4:line    |       |    5        | |       6         |  |7 |
		"[[:space:]]*([[:digit:]]*>)?(.*)(\\(([[:digit:]]+)\\)): (fatal error|error|warning) ([A-Z][[:digit:]]*): (.+)",
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

void NodeParser::init(std::string prjName, std::string prjFile, std::vector<std::string> systemIncs)
{
	auto s = splitFolderAndFile(prjFile);
	m_prjName = prjName;
	m_prjDir = s.first;
	m_prjFile = prjFile;
	m_systemIncs = systemIncs;
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

	if (!m_outer.m_fastParser && tryInclude(line))
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
		std::smatch matches;
		if (!std::regex_match(line, matches, m_outer.m_clNameRgx))
			return false;
	}

	m_currDefines.clear();
	m_currUserIncs.clear();

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
			m_currDefines.push_back(std::move(s));
		}
	}

	//
	// Extract all includes
	//
	auto addInc = [&](std::string s)
	{
		s = trim(s);
		CZ_CHECK(fullPath(s, s, m_prjDir));
		m_currUserIncs.push_back(std::move(s));
	};

	{
		static std::regex rgx("[[:space:]]\\/I\"([^\"]+)\"", std::regex::optimize);
		for (
			std::sregex_iterator i = std::sregex_iterator(line.begin(), line.end(), rgx);
			i != std::sregex_iterator();
			++i)
		{
			addInc((*i)[1]);
		}
	}
	{
		static std::regex rgx("[[:space:]]\\/I([^\\s,^\"]+)", std::regex::optimize);
		for (
			std::sregex_iterator i = std::sregex_iterator(line.begin(), line.end(), rgx);
			i != std::sregex_iterator();
			++i)
		{
			addInc((*i)[1]);
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
			//printf("*%s*\n", token.c_str());
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
			std::string fullpath;
			CZ_CHECK(fullPath(fullpath, *it, m_prjDir));
			if (m_outer.m_fastParser)
				triggerFastParser(fullpath);
			m_outer.m_db.addFile(
				fullpath, m_prjName, m_prjFile,
				joinDefines(m_currDefines),
				joinUserIncs(m_currUserIncs) + joinSystemIncludes(m_systemIncs),
				true);
		}

	}

	return true;
}

void NodeParser::triggerFastParser(const std::string& fullpath)
{
	auto includeDirs = std::make_shared<buildgraph::IncludeDirs>();
	includeDirs->addSystemInc(m_systemIncs);
	includeDirs->addUserInc(m_currUserIncs);
	includeDirs->pushParent(splitFolderAndFile(fullpath).first);
	m_outer.m_graph.processIncludes(
		buildgraph::Node::Type::Source, fullpath, includeDirs, m_currDefines, gAsync);
}

bool NodeParser::tryInclude(const std::string& line)
{
	if (m_state != State::ClCompile)
		return false;

	static std::regex rgx("Note: including file:[[:space:]]*(.*)", std::regex_constants::egrep | std::regex::optimize);
	std::smatch matches;
	if (!std::regex_match(line, matches, rgx))
		return false;

	auto fname = matches[1].str();

	if (!m_outer.m_updatedb)
		return true;

	CZ_CHECK(fullPath(fname, fname, m_prjDir));
	m_outer.m_db.addFile(
		fname, "", "" ,
		joinDefines(m_currDefines),
		joinUserIncs(m_currUserIncs) + joinSystemIncludes(m_systemIncs), true);
	return true;
}

} // namespace cz

