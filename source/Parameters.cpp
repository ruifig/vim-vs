#include "vimvsPCH.h"
#include "Parameters.h"

std::wstring Parameters::ms_empty;

Parameters::Parameters(Dummy)
{
	auto cmd = GetCommandLine();
	int argc;
	LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
	Set(argc, argv);
	LocalFree(argv);
}

void Parameters::Set(int argc, wchar_t* argv[])
{
	if (argc<=1)
		return;
	for (int i=1; i<argc; i++)
	{
		const wchar_t *arg = argv[i];
		if (*arg == '-')
			arg++;

		const wchar_t *seperator = wcschr(arg, '=');
		if (seperator==nullptr)
		{
			m_args.emplace_back(arg, L"");
		}
		else
		{
			std::wstring name(arg, seperator);
			std::wstring value(++seperator);
			m_args.emplace_back(std::move(name), std::move(value));
		}
	}
}

const Parameters::Param* Parameters::begin() const
{
	if (m_args.size())
		return &m_args[0];
	else
		return nullptr;
}

const Parameters::Param* Parameters::end() const
{
	return begin() + m_args.size();
}

bool Parameters::Has(const wchar_t* name ) const
{
	for(auto &i: m_args)
	{
		if (i.Name==name)
		{
			return true;
		}
	}
	return false;
}

bool Parameters::Has(const std::wstring& name) const
{
	return Has(name.c_str());
}

const std::wstring& Parameters::Get(const wchar_t *name) const
{
	for (auto &i: m_args)
	{
		if (i.Name==name)
			return i.Value;
	}
	return ms_empty;
}

const std::wstring& Parameters::Get(const std::wstring& name) const
{
	return Get(name.c_str());
}

#if 0
std::pair<bool,int> Parameters::GetAsInt(const wchar_t *name, int defaultVal) const
{
	std::pair<bool, int> res{ false, defaultVal};
	res.first = Has(name);
	if (res.first)
		res.second = std::atoi(ToUTF8(Get(name)).c_str());
	return res;
}

std::pair<bool,int> Parameters::GetAsInt(const std::wstring& name, int defaultVal) const
{
	return GetAsInt(name.c_str(), defaultVal);
}

#endif

int Parameters::Count() const
{
	return static_cast<int>(m_args.size());
}

void Parameters::Clear()
{
	m_args.clear();
}




