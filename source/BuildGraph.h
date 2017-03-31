#include <memory>
#include <vector>
#include <unordered_map>
#include <future>
#include "Logging.h"
#include "Utils.h"

namespace cz
{
namespace buildgraph
{

class IncludeDirs
{
public:
	IncludeDirs()
	{
		calcHash();
	}

	template<typename T>
	void addI(T&& v)
	{
		std::string str = std::forward<T>(v);
		ensureTrailingSlash(str);
		m_dirs.push_back(std::move(str));
		calcHash();
	}

	void addI(std::vector<std::string> v)
	{
		for (auto&& i : v)
			addI(std::move(i));
		calcHash();
	}

	void pushParent(std::string fullpath)
	{
		m_parents.push_back(std::move(fullpath));
		calcHash();
	}

	void popParent()
	{
		CZ_CHECK(m_parents.size());
		m_parents.pop_back();
		calcHash();
	}

	int getNumParents() const
	{
		return static_cast<int>(m_parents.size());
	}

	int64_t getHash() const { return m_hash; }

	std::string findHeader(const std::string& inc, bool quoted);

private:
	void calcHash()
	{
		std::string s;
		for(auto&& i : m_parents)
			s += i;
		for(auto&& i : m_dirs)
			s += i;
		m_hash = cz::hash(s);
	}

	std::vector<std::string> m_parents;
	std::vector<std::string> m_dirs;
	int64_t m_hash = 0;
};

class Graph;

//! The hash of a node is calculated on the lowercase of the name, so we can easily use a filename as a name
// This is because Windows filesystem is case insensitive.
class Node
{
public:
	explicit Node(std::string name)
		: m_name(std::move(name))
	{
		m_hash = hash(tolower(m_name));
	}

	//! If for some reason you already had the hash calculated, use this constructor, so it doesn't need to calculate
	// the hash again
	Node(std::string name, int64_t hash)
		: m_name(std::move(name))
	{
		m_hash = hash;
	}
private:
	friend Graph;
	int64_t m_hash = 0;
	std::string m_name;

	struct Data
	{
		// If this node is a source/header file, we use this to cache optimize multiple calls to detect includes
		// If a previous call was made to process includes using the same includeDirs as a previous call,
		// then we can skip the processing.
		std::unordered_map<int64_t, std::shared_ptr<IncludeDirs>> includeDirs;
		// dependencies
		std::unordered_map<int64_t, std::shared_ptr<Node>> deps;
	};
	Monitor<Data> data;
};

class Graph
{
public:
	std::shared_ptr<Node> getNode(const std::string& name, bool create=false);

	//! \param filename
	//		Full path, canonicalized
	void processIncludes(const std::string& filename, const std::shared_ptr<IncludeDirs>& includeDirs, bool async);

	void finishWork();

private:
	//! \param filename
	//		Full path, canonicalized
	void processIncludes(const std::shared_ptr<Node>& node, const std::shared_ptr<IncludeDirs>& includeDirs, bool async);

	struct Data
	{
		std::unordered_map<int64_t, std::shared_ptr<Node>> nodes;
		std::vector<std::future<void>> work;
	};
	Monitor<Data> m_data;
};


} // namespace buildgraph
} // namespace cz

