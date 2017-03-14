#pragma once

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
		std::wstring Name;
		std::wstring Value;
	};
	Parameters() {}
	Parameters(Dummy);
	void set(int argc,  wchar_t* argv[]);
	const Param* begin() const;
	const Param* end() const;
	bool has(const wchar_t* name) const;
	bool has(const std::wstring& name) const;
	const std::wstring& get(const wchar_t *name) const;
	const std::wstring& get(const std::wstring& name) const;

	int count() const;
	void clear();
private:
	static std::wstring ms_empty;
	std::vector<Param> m_args;
};

