#pragma once

namespace cz
{

class Parameters
{
public:
	enum Dummy
	{
		Auto
	};

	struct Param
	{
		template<class T1, class T2>
		Param(T1&& name_, T2&& value_)
			: Name(std::forward<T1>(name_))
			, Value(std::forward<T2>(value_)){}
		std::string Name;
		std::string Value;
	};
	Parameters() {}
	Parameters(Dummy);
	const Param* begin() const;
	const Param* end() const;
	bool has(const char* name) const;
	bool has(const std::string& name) const;
	const std::string& get(const char *name) const;
	const std::string& get(const std::string& name) const;

	int count() const;
	void clear();
private:
	void set(int argc,  wchar_t* argv[]);
	static std::string ms_empty;
	std::vector<Param> m_args;
};


} // namespace cz
