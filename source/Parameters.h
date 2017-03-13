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
	void Set(int argc,  wchar_t* argv[]);
	const Param* begin() const;
	const Param* end() const;
	bool Has(const wchar_t* name) const;
	bool Has(const std::wstring& name) const;
	const std::wstring& Get(const wchar_t *name) const;
	const std::wstring& Get(const std::wstring& name) const;

#if 0
	std::pair<bool, int> GetAsInt(const wchar_t *name, int defaultVal=0) const;
	std::pair<bool, int> GetAsInt(const std::wstring& name, int defaultVal=0) const;
#endif

	int Count() const;
	void Clear();
private:
	static std::wstring ms_empty;
	std::vector<Param> m_args;
};

