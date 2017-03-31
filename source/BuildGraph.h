#pragma once

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
	void addUserInc(T&& v)
	{
		std::string str = std::forward<T>(v);
		ensureTrailingSlash(str);
		m_userIncs.push_back(std::move(str));
		calcHash();
	}

	void addUserInc(std::vector<std::string> v)
	{
		for (auto&& i : v)
			addUserInc(std::move(i));
	}

	template<typename T>
	void addSystemInc(T&& v)
	{
		std::string str = std::forward<T>(v);
		ensureTrailingSlash(str);
		m_systemIncs.push_back(std::move(str));
		calcHash();
	}

	void addSystemInc(std::vector<std::string> v)
	{
		for (auto&& i : v)
			addSystemInc(std::move(i));
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

	auto& getSystemIncs() const { return m_systemIncs; }
	auto& getUserIncs() const { return m_userIncs; }

private:
	void calcHash()
	{
		std::string s;
		for(auto&& i : m_parents)
			s += i;
		for(auto&& i : m_userIncs)
			s += i;
		for(auto&& i : m_systemIncs)
			s += i;
		m_hash = cz::hash(s);
	}

	std::vector<std::string> m_parents;
	std::vector<std::string> m_userIncs;
	std::vector<std::string> m_systemIncs;
	int64_t m_hash = 0;
};

class Graph;

//! The hash of a node is calculated on the lowercase of the name, so we can easily use a filename as a name
// This is because Windows filesystem is case insensitive.
class Node
{
public:
	enum class Type
	{
		Source,
		Header
	};

	explicit Node(Type type, std::string name)
		: m_name(std::move(name))
		, m_type(type)
	{
		m_hash = hash(tolower(m_name));
	}

	//! If for some reason you already had the hash calculated, use this constructor, so it doesn't need to calculate
	// the hash again
	Node(Type type, std::string name, int64_t hash)
		: m_name(std::move(name))
		, m_type(type)
	{
		m_hash = hash;
	}

	const std::string& getName() const
	{
		return m_name;
	}

	Type getType() const
	{
		return m_type;
	}

	std::shared_ptr<IncludeDirs> getIncludeDirs()
	{
		return m_data([](Data& data) -> std::shared_ptr<IncludeDirs>
		{
			if (data.includeDirs.size())
				return data.includeDirs.begin()->second;
			else
				return nullptr;
		});
	}

	std::vector<std::string> getDefines()
	{
		return m_data([](Data& data)
		{
			return data.defines;
		});
	}

private:
	friend Graph;
	std::string m_name;
	Type m_type;
	int64_t m_hash = 0;

	struct Data
	{
		std::vector<std::string> defines;
		// If this node is a source/header file, we use this to cache optimize multiple calls to detect includes
		// If a previous call was made to process includes using the same includeDirs as a previous call,
		// then we can skip the processing.
		std::unordered_map<int64_t, std::shared_ptr<IncludeDirs>> includeDirs;
		// dependencies
		std::unordered_map<int64_t, std::shared_ptr<Node>> deps;
	};
	Monitor<Data> m_data;
};

class Graph
{
public:
	std::shared_ptr<Node> getNode(Node::Type type, const std::string& name, bool create=false);

	//! \param filename
	//		Full path, canonicalized
	void processIncludes(Node::Type type, const std::string& filename, const std::shared_ptr<IncludeDirs>& includeDirs,
		std::vector<std::string> defines, bool async);
	void finishWork();

	template<typename F>
	void iterate(F&& f)
	{
		return m_data([&](Data& data)
		{
			for (auto&& n : data.nodes)
				f(n.second);
		});
	}

private:
	//! \param filename
	//		Full path, canonicalized
	void processIncludes(const std::shared_ptr<Node>& node, const std::shared_ptr<IncludeDirs>& includeDirs,
		std::vector<std::string> defines, bool async);
	static bool canSkipProcessIncludes(const std::shared_ptr<Node>& node, const std::shared_ptr<IncludeDirs>& includeDirs,
		const std::vector<std::string>& defines);

	struct Data
	{
		std::unordered_map<int64_t, std::shared_ptr<Node>> nodes;
		std::vector<std::future<void>> work;
	};
	Monitor<Data> m_data;
};


} // namespace buildgraph
} // namespace cz

