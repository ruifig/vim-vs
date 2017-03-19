#include "vimvsPCH.h"
#include "Database.h"
#include "Logging.h"
#include "Utils.h"
#define MURMUR_SEED 'vivs'

namespace cz
{

Database::Database()
{
}

static int64_t hash(const std::wstring& s)
{
	int64_t out[2]; 
	MurmurHash3_x64_128(s.data(), static_cast<int>(s.size() * sizeof(s[0])), MURMUR_SEED, out);
	return out[0];
}

std::wstring tolower(const std::wstring& s)
{
	auto r = s;
	_wcslwr(&r[0]);
	return r;
}

bool Database::open(const std::wstring& dbfname)
{
	bool createDb = !isExistingFile(dbfname);
	if (createDb)
		CZ_LOG(logDefault, Log, L"Creating database '%s'", dbfname.c_str());
	CZ_CHECK(m_sqdb.open(toUTF8(dbfname).c_str(), createDb));

	//
	// Create necessary tables
	//
	if (createDb)
	{
		SqStmt sql;
		sql.init(m_sqdb, " \
			CREATE TABLE test ( \
				id            INTEGER PRIMARY KEY, \
				fullpath      VARCHAR, \
				name          VARCHAR, \
				projectName   VARCHAR, \
				configuration VARCHAR, \
				defines       VARCHAR, \
				includes      VARCHAR \
			); \
		");

		CZ_CHECK(sql.exec());
	}

	CZ_CHECK(m_sqlGetFile.init(m_sqdb, "SELECT * FROM files WHERE id=?"));
	CZ_CHECK(m_sqlAddFile.init(m_sqdb, "INSERT INTO files(id,fullpath,name,projectName,configuration,defines,includes) VALUES(?,?,?,?,?,?,?)"));

	SourceFile f;
	f.fullpath = L"C:/aa\\Bb\\cC.txt";
	getSourceFile(f);

	return true;
}

bool Database::getSourceFile(SourceFile& out)
{
	auto f = out.fullpath;
	_wcslwr(&f[0]);

	if (!out.id)
	{
		CZ_CHECK(out.fullpath.size());
		out.id = hash(tolower(out.fullpath));
	}

	CZ_CHECK(m_sqlGetFile.bindInt64(1, out.id));

	bool found = false;
	bool done = m_sqlGetFile.exec<int64_t, const char*, const char*, const char*, const char*, const char*, const char*>(
		[&](int64_t id, const char* fullpath, const char* name, const char* projectName, const char* configuration, const char* defines, const char* includes)
	{
		CZ_ASSERT(out.id == id);
		out.fullpath = toUTF16(fullpath);
		out.name = toUTF16(name);
		out.projectName = toUTF16(projectName);
		out.configuration = toUTF16(configuration);
		out.defines = toUTF16(defines);
		out.includes = toUTF16(includes);
		found = true;
		return true;
	});

	return found;
}

void Database::addFile(File file)
{
	wprintf(L"%s: %s\n", file.prjName.c_str(), file.name.c_str());

	SourceFile src;
	src.id = hash(tolower(file.name));
	if (!getSourceFile(src))
	{
		CZ_CHECK(m_sqlAddFile.bindInt64(1, src.id));
		CZ_CHECK(m_sqlAddFile.bindText(2, toUTF8(file.name)));
		CZ_CHECK(m_sqlAddFile.bindText(3, toUTF8(splitFolderAndFile(file.name).second)));
		CZ_CHECK(m_sqlAddFile.bindText(4, toUTF8(file.prjName)));
		CZ_CHECK(m_sqlAddFile.bindText(5, toUTF8(L""))); // configuration
		CZ_CHECK(m_sqlAddFile.bindText(6, file.params->getReadyParams())); // configuration
		CZ_CHECK(m_sqlAddFile.bindText(7, file.systemIncludes->getIncludes()));
		CZ_CHECK(m_sqlAddFile.exec());
	}

	m_files[file.name] = std::move(file);
}

cz::File* Database::getFile(const std::wstring& filename)
{
	auto it = m_files.find(filename);
	return it == m_files.end() ? nullptr : &it->second;
}

}

