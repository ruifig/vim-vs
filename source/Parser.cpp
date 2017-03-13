#include "vimvsPCH.h"
#include "Parser.h"

namespace cz
{

//////////////////////////////////////////////////////////////////////////
//		Parser
//////////////////////////////////////////////////////////////////////////
Parser::Parser(Database& db)
	: m_db(db)
{
}

void Parser::inject(const std::wstring& data)
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
				parse(m_line);
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

bool Parser::tryVimVsBegin(std::wstring& line)
{
	static std::wregex rgx(
		L"[[:space:]]*rem vim-vs-begin: ProjectName=\"(.+)\", ProjectFileName=\"(.+)\", ProjectDir=\"(.+)\", IncludePath=(.+)",
		std::regex_constants::egrep | std::regex::optimize);
	std::wsmatch matches;
	if (!std::regex_match(line, matches, rgx))
		return false;

	auto systemIncludes = std::make_shared<SystemIncludes>();

	auto projectName = matches[1].str();
	auto projectFileName = matches[2].str();
	auto projectDir = matches[3].str();
	auto includePath = matches[4].str();

	size_t s = 0;
	size_t e = 0;
	while (e != std::wstring::npos)
	{
		e = includePath.find(';', s);
		std::wstring str;
		if (e == std::wstring::npos)
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
	node->init(projectName, projectFileName, projectDir, systemIncludes);

	return true;
}

bool Parser::tryVimVsEnd(std::wstring& line)
{
	static std::wregex rgx(
		L"[[:space:]]*rem vim-vs-end: ProjectName=\"(.+)\"",
		std::regex_constants::egrep | std::regex::optimize);
	std::wsmatch matches;
	if (!std::regex_match(line, matches, rgx))
		return false;

	auto projectName = matches[1].str();
	auto it = m_nodes.find(m_currNode);
	if (it == m_nodes.end())
	{
		printf("");
	}
	if (it->second->getName() != projectName)
	{
		printf("");
	}

	CZ_CHECK(it != m_nodes.end() && it->second->getName() == projectName);
	it->second->finish();

	return true;
}

void Parser::parse(std::wstring line)
{
	if (m_currNode==0)
	{
		static std::wregex rgx(L"[[:space:]]*(1>)?Project \".*\" on node 1.*", std::regex_constants::egrep | std::regex::optimize);
		std::wsmatch matches;
		if (!std::regex_match(line, matches, rgx))
			return;
		m_mp = matches[1].matched;
		m_currNode = 1;
		return;
	}

	// Detect what node to pass this to
	// Remove the "N>" if present
	static std::wregex rgx(L"[[:space:]]*(([[:digit:]]+)>)?(.*)", std::regex_constants::egrep | std::regex::optimize);
	std::wsmatch matches;
	CZ_CHECK(std::regex_match(line, matches, rgx));
	if (matches[2].matched) // The node number
	{
		auto n = std::stoi(matches[2].str()); // # TODO : Remove this
		m_currNode = std::stoi(matches[2].str());
	}

	line = matches[3].str();
	if (tryVimVsBegin(line))
		return;
	if (tryVimVsEnd(line))
		return;

	auto it = m_nodes.find(m_currNode);
	if (it != m_nodes.end() && !it->second->isFinished())
	{
		it->second->parseLine(line);
	}

}

//////////////////////////////////////////////////////////////////////////
//		NodeParser
//////////////////////////////////////////////////////////////////////////
NodeParser::NodeParser(Parser& outer) : m_outer(outer)
{
}

void NodeParser::init(std::wstring prjName, std::wstring prjFileName, std::wstring prjPath, std::shared_ptr<SystemIncludes> systemIncludes)
{
	m_prjName = prjName;
	m_prjFileName = prjFileName;
	m_prjPath = prjPath;
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

const std::wstring& NodeParser::getName() const
{
	return m_prjName;
}

void NodeParser::parseLine(const std::wstring& line)
{
	if (tryCompile(line))
		return;

	if (tryInclude(line))
		return;
}

bool NodeParser::tryCompile(const std::wstring& line)
{
	if (trim(line) == L"ClCompile:")
	{
		m_state = State::ClCompile;
		return true;
	}
	if (m_state != State::ClCompile)
		return false;

	// Check if its a call to cl.exe
	{
		const wchar_t* re = L".*\\\\CL\\.exe .*";
		static std::wregex rgx(re, std::regex_constants::egrep | std::regex::optimize);
		std::wsmatch matches;
		if (!std::regex_match(line, matches, rgx))
			return false;
	}

	m_currClCompileParams = std::make_shared<Params>();

	//
	// Extract all defines
	//
	{
		static std::wregex rgx(L"\\/D \"?([[:graph:]]*)\"?", std::regex::optimize);
		for (
			std::wsregex_iterator i = std::wsregex_iterator(line.begin(), line.end(), rgx);
			i != std::wsregex_iterator();
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
			m_currClCompileParams->defines.push_back(s);
		}
	}

	//
	// Extract all includes
	//
	{
		wprintf(L"%s\n", line.c_str());
		//static std::wregex rgx(L"\\/I\"([[:graph:]]*)\"", std::regex::optimize);
		static std::wregex rgx(L"[[:space:]]\\/I\"([^\"]+)\"", std::regex::optimize);
		for (
			std::wsregex_iterator i = std::wsregex_iterator(line.begin(), line.end(), rgx);
			i != std::wsregex_iterator();
			++i)
		{
			auto&& m = (*i)[1];
			m_currClCompileParams->includes.push_back(replace(m.str(), '\\', '/'));
		}
	}
	{
		//static std::wregex rgx(L"\\/I\"([[:graph:]]*)\"", std::regex::optimize);
		static std::wregex rgx(L"[[:space:]]\\/I([^\\s,^\"]+)", std::regex::optimize);
		for (
			std::wsregex_iterator i = std::wsregex_iterator(line.begin(), line.end(), rgx);
			i != std::wsregex_iterator();
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
	{
		std::vector<std::wstring> tokens;
		// e : points to the last char in the token
		auto e = line.end() - 1;
		while (true)
		{
			while (*e == ' ') e--; // remove spaces
			bool quoted = *e == '"' ? true : false;
			if (quoted) e--; // Remove the "

			// s : Points to one character behind the token start
			std::wstring::const_iterator s;
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

			auto token = std::wstring(s + 1, e + 1);

			// Exit conditions are:
			// - Token starts with /
			//		- Additional, if token is a /D, then also drop the previously processor token (which is the macro)
			wprintf(L"*%s*\n", token.c_str());
			if (*token.begin() == '/')
			{
				if (token == L"/D")
					tokens.pop_back();
				break;
			}
			tokens.push_back(std::move(token));
			e = s - 1;
		}

		for (auto it = tokens.rbegin(); it != tokens.rend(); ++it)
		{
			File f;
			CZ_CHECK(fullPath(f.name, *it, m_prjPath));
			f.prjName = m_prjName;
			f.systemIncludes = m_systemIncludes;
			f.params = m_currClCompileParams;
			m_outer.m_db.addFile(std::move(f));
		}
	}

	return true;
}

bool NodeParser::tryInclude(const std::wstring& line)
{
	if (m_state != State::ClCompile)
		return false;

	static std::wregex rgx(L"Note: including file:[[:space:]]*(.*)", std::regex_constants::egrep | std::regex::optimize);
	std::wsmatch matches;
	if (!std::regex_match(line, matches, rgx))
		return false;

	// At this point, we should have compile parameters set, since we are compiling a file
	assert(m_currClCompileParams);
	auto fname = matches[1].str();

	// Check if this file was already in the database
	if (m_outer.m_db.getFile(fname) != nullptr)
		return true;

	File f;
	f.name = fname;
	f.prjName = m_prjName;
	f.systemIncludes = m_systemIncludes;
	f.params = m_currClCompileParams;
	m_outer.m_db.addFile(std::move(f));
	return true;
}

} // namespace cz

