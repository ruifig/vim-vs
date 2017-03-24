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

static int64_t hash(const std::string& s)
{
	int64_t out[2]; 
	MurmurHash3_x64_128(s.data(), static_cast<int>(s.size() * sizeof(s[0])), MURMUR_SEED, out);
	return out[0];
}

bool Database::open(const std::string& dbfname)
{
	bool createDb = !isExistingFile(dbfname);
	if (createDb)
		CZ_LOG(logDefault, Log, "Creating database '%s'", dbfname.c_str());
	CZ_CHECK(m_sqdb.open(dbfname.c_str(), createDb));

	SqStmt optimize;
	optimize.init(m_sqdb, "PRAGMA synchronous = OFF");
	CZ_CHECK(optimize.exec());

	//
	// Create necessary tables
	//
	if (createDb)
	{
		SqStmt sql;
		sql.init(m_sqdb, " \
			CREATE TABLE files ( \
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

	return true;
}

bool Database::getSourceFile(SourceFile& out)
{
	auto fid = out.id;
	if (!fid)
	{
		CZ_CHECK(out.fullpath.size());
		fid = hash(tolower(out.fullpath));
	}

	CZ_CHECK(m_sqlGetFile.bindInt64(1, fid));

	bool found = false;
	m_sqlGetFile.exec<int64_t, const char*, const char*, const char*, const char*, const char*, const char*>(
		[&](int64_t id, const char* fullpath, const char* name, const char* project, const char* configuration, const char* defines, const char* includes)
	{
		CZ_ASSERT(fid == id);
		out.id = id;
		out.fullpath = fullpath;
		out.name = name;
		out.project = project;
		out.configuration = configuration;
		out.defines = defines;
		out.includes = includes;
		found = true;
		return true;
	});

	return found;
}

void Database::addFile(File file)
{
	SourceFile src;
	src.id = hash(tolower(file.name));
	if (!getSourceFile(src))
	{
		auto basename = splitFolderAndFile(file.name).second;
		CZ_LOG(logDefault, Log, "Adding file %s to database: id=%llu, fullpath=\"%s\", prj=%s, %s|%s",
			basename.c_str(), src.id, file.name.c_str(), file.project.c_str(),
			file.params->getReadyParamsDB().c_str(), file.systemIncludes->getIncludesDB().c_str());

		CZ_CHECK(m_sqlAddFile.bindInt64(1, src.id));
		CZ_CHECK(m_sqlAddFile.bindText(2, file.name));
		CZ_CHECK(m_sqlAddFile.bindText(3, basename));
		CZ_CHECK(m_sqlAddFile.bindText(4, file.project));
		CZ_CHECK(m_sqlAddFile.bindText(5, "")); // configuration
		CZ_CHECK(m_sqlAddFile.bindText(6, file.params->getReadyParamsDB()));
		CZ_CHECK(m_sqlAddFile.bindText(7, file.systemIncludes->getIncludesDB()));
		CZ_CHECK(m_sqlAddFile.exec());
	}

	m_files[file.name] = std::move(file);
}

cz::File* Database::getFile(const std::string& filename)
{
	auto it = m_files.find(filename);
	return it == m_files.end() ? nullptr : &it->second;
}

SourceFile Database::getSourceFile(const std::string& filename)
{
	SourceFile s;
	s.fullpath = filename;
	getSourceFile(s);
	return s;
}

}

