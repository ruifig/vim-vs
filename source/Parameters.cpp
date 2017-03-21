#include "vimvsPCH.h"
#include "Parameters.h"
#include "Utils.h"

namespace cz
{

std::string Parameters::ms_empty;

Parameters::Parameters(Dummy)
{
	auto cmd = GetCommandLineW();
	int argc;
	LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
	set(argc, argv);
	LocalFree(argv);
}

void Parameters::set(int argc, wchar_t* argv[])
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
			m_args.emplace_back(narrow(arg), "");
		}
		else
		{
			std::wstring name(arg, seperator);
			std::wstring value(++seperator);
			m_args.emplace_back(narrow(name), narrow(value));
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

bool Parameters::has(const char* name ) const
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

bool Parameters::has(const std::string& name) const
{
	return has(name.c_str());
}

const std::string& Parameters::get(const char *name) const
{
	for (auto &i: m_args)
	{
		if (i.Name==name)
			return i.Value;
	}
	return ms_empty;
}

const std::string& Parameters::get(const std::string& name) const
{
	return get(name.c_str());
}

int Parameters::count() const
{
	return static_cast<int>(m_args.size());
}

void Parameters::clear()
{
	m_args.clear();
}


} // namespace cz
