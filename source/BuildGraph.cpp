#include "vimvsPCH.h"
#include "BuildGraph.h"
#include "Utils.h"

namespace cz
{

CZ_DECLARE_LOG_CATEGORY(logBuildGraph, Log, Log)
CZ_DEFINE_LOG_CATEGORY(logBuildGraph)

namespace buildgraph
{

//
//- Relative paths passed to CL.exe are relative to the project's file path
//- CL.exe include search rules (https://msdn.microsoft.com/en-us/library/36k2cdd4.aspx)
//Quoted form:
//The preprocessor searches for include files in this order:
//	1) In the same directory as the file that contains the #include statement.
//	2) In the directories of the currently opened include files, in the reverse order in which they were opened. The search begins in the directory of the parent include file and continues upward through the directories of any grandparent include files.
//	3) Along the path that's specified by each /I compiler option.
//	4) Along the paths that are specified by the INCLUDE environment variable.
//
//Angle-bracket form:
//The preprocessor searches for include files in this order:
//	1) Along the path that's specified by each /I compiler option.
//	2) When compiling occurs on the command line, along the paths that are specified by the INCLUDE environment variable.
std::string IncludeDirs::findHeader(const std::string& inc, bool quoted)
{
	if (quoted)
	{
		for (auto i = m_parents.rbegin(); i != m_parents.rend(); ++i )
		{ 
			auto f = *i + inc;
			CZ_CHECK(fullPath(f, f, ""));
			if (isExistingFile(f))
			{
				return f;
			}
		} 
	}

	for(auto&& i : m_dirs)
	{
		auto f = i + inc;
		CZ_CHECK(fullPath(f, f, ""));
		if (isExistingFile(f))
		{
			return f;
		}
	}

	return "";
}

std::shared_ptr<Node> Graph::getNode(const std::string& name, bool create)
{
	int64_t key = hash(tolower(name));
	return m_data([&](Data& data) -> std::shared_ptr<Node>
	{
		auto it = data.nodes.find(key);
		if (it!=data.nodes.end())
			return it->second;
		if (!create)
			return nullptr;
		auto node = std::make_shared<Node>(name, key);
		data.nodes.emplace(key, node);
		return node;
	});
}

void Graph::processIncludes(const std::string& filename, const std::shared_ptr<IncludeDirs>& includeDirs, bool async)
{
	if (!isExistingFile(filename))
	{
		CZ_LOG(logBuildGraph, Error, "File '%s' not found", filename.c_str());
		return;
	}

	auto node = getNode(filename, true);
	processIncludes(node, includeDirs, async);
}

void Graph::processIncludes(const std::shared_ptr<Node>& node, const std::shared_ptr<IncludeDirs>& includeDirs, bool async)
{
	// If this file was already process with the specified include directories, then any #includes in the file
	// will lead to the same headers, therefore nothing changing. So we can skip this
	auto skip = node->data([&](Node::Data& data) -> bool
	{
		if (data.includeDirs.find(includeDirs->getHash())!=data.includeDirs.end())
			return true;
		data.includeDirs.emplace(includeDirs->getHash(), includeDirs);
		return false;
	});

	if (skip)
		return;

	std::ifstream file(widen(node->m_name));
	if (!file.is_open())
	{
		auto msg = formatString("Could not open file '%s'", node->m_name.c_str());
		fprintf(stderr, "%s\n", msg);
		CZ_LOG(logBuildGraph, Fatal, msg);
		return;
	}

	//                                                                    1     2    3
	static std::regex rgx("^[[:space:]]*#[[:space:]]*include[[:space:]]*(\"|<)(.+)(\"|>)", std::regex::optimize);
	std::string line;
	std::vector<std::future<void>> work;
	std::string folder = splitFolderAndFile(node->m_name).first;
	while(std::getline(file, line))
	{
		std::smatch matches;
		if (!std::regex_match(line,matches, rgx))
			continue;

		auto quoted = matches[1].str()=="\"";
		auto inc = matches[2].str();
		auto otherName = includeDirs->findHeader(inc, quoted);
		if (otherName=="") 
		{
			// header file not found
			CZ_LOG(logBuildGraph, Warning, "Failed to find header '%s' in file '%s'", inc.c_str(), node->m_name.c_str());
			continue;
		}
		printf("%0*d%s\n", includeDirs->getNumParents(), 0, otherName.c_str());
		auto otherFolder = splitFolderAndFile(otherName).first;
		std::shared_ptr<IncludeDirs> otherIncludeDirs;
		if (otherFolder==folder)
		{
			otherIncludeDirs = includeDirs;
		}
		else
		{
			otherIncludeDirs = std::make_shared<IncludeDirs>();
			*otherIncludeDirs = *includeDirs;
			otherIncludeDirs->pushParent(otherFolder);
		}

		auto otherNode = getNode(otherName, true);
		node->data([&otherNode](Node::Data& data)
		{
			CZ_ASSERT(data.deps.find(otherNode->m_hash) == data.deps.end());
			data.deps.emplace(otherNode->m_hash, otherNode);
		});

		auto ft = std::async(
			async ? std::launch::async : std::launch::deferred,
			[this, otherNode, otherIncludeDirs, async]()
			{
				processIncludes(otherNode, otherIncludeDirs, async);
			});
		work.push_back(std::move(ft));
	}

	m_data([&](Data& data)
	{
		moveAppend(work, data.work);
	});
}

void Graph::finishWork()
{
	while (true)
	{
		std::vector<std::future<void>> work;
		m_data([&](Data& data)
		{
			work = std::move(data.work);
			data.work.clear();
		});

		if (work.size() == 0)
			return;
		// This blocks until all futures complete
		for (auto&& w : work)
			w.get();
	}
}

} // namespace buildgraph
} // namespace cz

