/********************************************************************
	CrazyGaze (http://www.crazygaze.com)
	Author : Rui Figueira
	Email  : rui@crazygaze.com

	purpose:

*********************************************************************/

#pragma once

namespace cz
{

	class IniFile
	{
	public:

		class Entry;

		class Section
		{
		public:
			Section()
			{
			}
			~Section()
			{
			}

			const std::string& getName() const
			{
				return m_name;
			}

			int getNumEntries() const
			{
				return static_cast<int>(m_entries.size());
			}

			Entry* getEntry(int index)
			{
				return &m_entries[index];
			}

			Entry* getEntry(const char* name, bool bCreate = true);

			template<typename T>
			Entry* getEntryWithDefault(const char* name, T defaultValue)
			{
				auto entry = getEntry(name);
				if (entry->m_val.size() == 0)
					entry->setValue(defaultValue);
				return entry;
			}

			//! Changes the value of an existing entry, or creates a new one if necessary
			void setValue(const char* szEntryName, const char* szValue);
			//! Changes the value of an existing entry, or creates a new one if necessary
			void setValue(const char* szEntryName, int val);
			//! Changes the value of an existing entry, or creates a new one if necessary
			void setValue(const char* szEntryName, float val);

			//! Adds a new entry/value pair, even if it creates a duplicated entry name
			void add(const char* szEntryName, const char* szValue);
			//! Adds a new entry/value pair, even if it creates a duplicated entry name
			void add(const char* szEntryName, int val);
			//! Adds a new entry/value pair, even if it creates a duplicated entry name
			void add(const char* szEntryName, float val);

		protected:
			friend class IniFile;
			void init(const char* name);

		private:
			std::string m_name;
			std::vector<Entry> m_entries;
		};


		class Entry
		{
		public:
			Entry()
			{
			}

			Entry(const Entry& other)
				: m_name(other.m_name), m_val(other.m_val)
			{
			}

			~Entry()
			{
			}

			const std::string& getName() const
			{
				return m_name;
			}

			const std::string& asString() const
			{
				return m_val;
			}

			int asInt() const;
			float asFloat() const;
			bool asBoolean() const;

			template<typename T> T as();
			template<> int as()
			{
				return asInt();
			}
			template<> bool as()
			{
				return asBoolean();
			}
			template<> float as()
			{
				return asFloat();
			}
			template<> const char* as()
			{
				return asString().c_str();
			}
			template<> std::string as()
			{
				return asString();
			}

			bool operator==(const Entry& other) const
			{
				return m_name == other.m_name;
			}

		protected:
			friend class IniFile::Section;
			void init(const char* name, const char* val);
			void setValue(const char* val);
			void setValue(bool val);
			void setValue(int val);
			void setValue(float val);

		private:
			std::string m_name;
			std::string m_val;
		};

		IniFile() {}
		virtual ~IniFile();
		bool open(const char* filename);

		int getNumSections() const
		{
			return static_cast<int>(m_sections.size());
		}

		Section* getSection(int index)
		{
			return m_sections[index].get();
		}

		Section* getSection(const char* szName, bool bCreate = true);

		template<typename T>
		T getValue(const char* szSection, const char* szName, T defaultVal)
		{
			return getSection(szSection)->getEntryWithDefault(szName, defaultVal)->as<T>();
		}

	private:
		std::vector<std::unique_ptr<Section>> m_sections;
	};

} // namespace cz

