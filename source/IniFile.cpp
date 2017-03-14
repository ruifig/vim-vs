/********************************************************************
	CrazyGaze (http://www.crazygaze.com)
	Author : Rui Figueira
	Email  : rui@crazygaze.com
	
	purpose:
	
*********************************************************************/

#include "vimvsPCH.h"
#include "IniFile.h"
#include "Utils.h"
#include "Logging.h"

namespace cz
{

static void stringSplitIntoLines(const wchar_t* textbuffer, int buffersize, std::vector<std::wstring> *lines)
{
	lines->clear();
	if (*textbuffer == 0)
		return;

	const wchar_t* s = textbuffer;
	while (*s != 0 && s < textbuffer + buffersize)
	{
		const wchar_t* ptrToChar = s;
		while (!(*s == 0 || *s == 0xA || *s == 0xD))
			s++;

		size_t numchars = s - ptrToChar;
		lines->emplace_back(ptrToChar, ptrToChar + numchars);

		// New lines format are:
		// Unix		: 0xA
		// Mac		: 0xD
		// Windows	: 0xD 0xA
		// If windows format a new line has 0xD 0xA, so we need to skip one extra character
		if (*s == 0xD && *(s + 1) == 0xA)
			s++;

		if (*s == 0)
			break;

		s++; // skip the newline character
	}
}

//////////////////////////////////////////////////////////////////////////
// Entry
//////////////////////////////////////////////////////////////////////////
void IniFile::Entry::init( const wchar_t* name, const wchar_t* val )
{
	m_name = name;
	m_val = val;
}

void IniFile::Entry::setValue( const wchar_t* val )
{
	m_val = val;
}

void IniFile::Entry::setValue(bool val)
{
	m_val = val ? L"true" : L"false";
}

void IniFile::Entry::setValue(int val)
{
	std::wostringstream ostr;
	ostr << val;
	m_val = ostr.str().c_str();
}

void IniFile::Entry::setValue(float val)
{
	std::wostringstream ostr;
	ostr << val;
	m_val = ostr.str().c_str();
}

int IniFile::Entry::asInt() const
{
	int val = std::stoi(m_val.c_str());
	return val;
}

bool IniFile::Entry::asBoolean() const
{
	if (m_val==L"1" || m_val==L"true" || m_val==L"True" || m_val==L"TRUE")
		return true;
	else
		return false;
}

float IniFile::Entry::asFloat() const
{
	return static_cast<float>(std::stof(m_val.c_str()));
}

//////////////////////////////////////////////////////////////////////////
// Section
//////////////////////////////////////////////////////////////////////////

void IniFile::Section::init(const wchar_t* name)
{
	m_name = name;
}

IniFile::Entry* IniFile::Section::getEntry(const wchar_t* name, bool bCreate)
{
	for(auto&& e : m_entries)
	{
		if (e.getName() == name)
			return &e;
	}

	if (bCreate)
	{
		m_entries.push_back(IniFile::Entry());
		m_entries.back().init(name, L"");
		return &m_entries.back();
	}
	else
	{
		return NULL;
	}

}

void IniFile::Section::setValue(const wchar_t* szEntryName, const wchar_t* szValue)
{
	IniFile::Entry* pEntry = getEntry(szEntryName);
	pEntry->setValue(szValue);
}

void IniFile::Section::setValue(const wchar_t* szEntryName, int val)
{
	IniFile::Entry* pEntry = getEntry(szEntryName);
	pEntry->setValue(val);
}

void IniFile::Section::setValue(const wchar_t* szEntryName, float val)
{
	IniFile::Entry* pEntry = getEntry(szEntryName);
	pEntry->setValue(val);
}

void IniFile::Section::add(const wchar_t* szEntryName, const wchar_t* szValue)
{
	m_entries.push_back(IniFile::Entry());
	m_entries.back().init(szEntryName, szValue);
}
void IniFile::Section::add(const wchar_t* szEntryName, int val)
{
	m_entries.push_back(IniFile::Entry());
	m_entries.back().init(szEntryName, L"");	
	m_entries.back().setValue(val);
}
void IniFile::Section::add(const wchar_t* szEntryName, float val)
{
	m_entries.push_back(IniFile::Entry());
	m_entries.back().init(szEntryName, L"");	
	m_entries.back().setValue(val);
}

//////////////////////////////////////////////////////////////////////////
// IniFile
//////////////////////////////////////////////////////////////////////////

IniFile::~IniFile()
{
}

bool IniFile::open(const wchar_t* filename)
{
	std::ifstream ifs(filename);
	if (!ifs.is_open())
	{
		CZ_LOG(logDefault, Warning, L"Error opening ini file %s", filename);
		return false;
	}

	auto text = widen(std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()));
	std::vector<std::wstring> lines;
	stringSplitIntoLines(text.c_str(), static_cast<int>(text.size()), &lines);

	std::wstring tmp1, tmp2, tmp3;

	for(auto&& line : lines)
	{
		line = trim(line);
		std::wstring::iterator it;

		if (line.c_str()[0]==';' || line.c_str()[0]=='#' )
		{
			// Its a comment
		}
		else if (line.c_str()[0]=='[') // its a section
		{
			tmp1 = L"";
			tmp1.append(line.begin()+1, line.end()-1);
			tmp1 = trim(tmp1);
			auto section = std::make_unique<Section>();
			section->init(tmp1.c_str());
			m_sections.push_back(std::move(section));
		}
		else if ( (it=std::find(line.begin(), line.end(), '='))!=line.end()) // its a value
		{
			if (m_sections.size())
			{
				tmp1 = L"";
				tmp1.append(line.begin(), it);
				tmp1 = trim(tmp1);
				tmp2 = L"";
				tmp2.append(it+1,line.end());
				tmp2 = trim(tmp2);
				if (*tmp2.begin()=='"' || *tmp2.begin()=='\'')
				{
					tmp3 = L"";
					tmp3.append(tmp2.begin()+1, tmp2.end()-1);
					m_sections.back()->add(tmp1.c_str(), tmp3.c_str());
				}
				else
				{
					m_sections.back()->add(tmp1.c_str(), tmp2.c_str());
				}

			}
			else
			{
				CZ_LOG(logDefault, Warning, L"Value with no section in INI File (%s) : %s ", filename, line.c_str());
			}
		}
		else if (line==L"")
		{

		}
		else
		{
			CZ_LOG(logDefault, Warning, L"Invalid line in INI File (%s) : %s ", filename, line.c_str());
		}
	}

	return true;
}

IniFile::Section* IniFile::getSection(const wchar_t* szName, bool bCreate)
{
	for (auto&& s : m_sections)
	{
		if (s->getName() == szName)
			return s.get();
	}

	if (bCreate)
	{
		m_sections.push_back(std::make_unique<Section>());
		m_sections.back()->init(szName);
		return m_sections.back().get();
	}

	return NULL;
}

} // namespace cz

