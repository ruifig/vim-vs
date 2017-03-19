/********************************************************************
	CrazyGaze (http://www.crazygaze.com)
	Author : Rui Figueira
	Email  : rui@crazygaze.com
	
	purpose:
	
*********************************************************************/

#include "vimvsPCH.h"
#include "SqLiteWrapper.h"
#include "Logging.h"

namespace cz
{

SqDatabase::~SqDatabase()
{
	if (m_db)
	{
		sqlite3_close(m_db);
	}
}

bool SqDatabase::open(const char* database, bool create)
{
	int flags = SQLITE_OPEN_READWRITE;
	if (create)
		flags |= SQLITE_OPEN_CREATE;
	if (sqlite3_open_v2(database, &m_db, flags, NULL)!=SQLITE_OK)
	{
		CZ_LOG(logDefault, Error, L"%s", toUTF16(sqlite3_errmsg(m_db)).c_str());
		return false;
	}
	
	return true;
}

void SqDatabase::transaction_begin()
{
	int rc = sqlite3_exec(m_db, "BEGIN", 0,0,0);
	CZ_ASSERT(rc==SQLITE_OK);
}

void SqDatabase::transaction_rollback()
{
	int rc = sqlite3_exec(m_db, "ROLLBACK", 0,0,0);
	CZ_ASSERT(rc==SQLITE_OK);
}

void SqDatabase::transaction_commit()
{
	int rc = sqlite3_exec(m_db, "COMMIT", 0,0,0);
	CZ_ASSERT(rc==SQLITE_OK);
}

uint64_t SqDatabase::last_insert_rowid()
{
	return sqlite3_last_insert_rowid(m_db);
}

//////////////////////////////////////////////////////////////////////////
//
//			SqTransaction
//
//////////////////////////////////////////////////////////////////////////
SqTransaction::SqTransaction(SqDatabase &db) : m_db(db), m_commited(false)
{
	m_db.transaction_begin();
}
SqTransaction::~SqTransaction()
{
	if (!m_commited)
		m_db.transaction_rollback();
}

void SqTransaction::commit()
{
	CZ_ASSERT(m_commited==false);
	m_db.transaction_commit();
	m_commited = true;
}

//////////////////////////////////////////////////////////////////////////
//
//			SqStmt
//
//////////////////////////////////////////////////////////////////////////

SqStmt::SqStmt()
{
	m_db = nullptr;
	m_stmt = nullptr;
}

bool SqStmt::init(sqlite3* db, const char* sql)
{
	m_db = db;
	int rc = sqlite3_prepare_v2(m_db, sql, -1, &m_stmt, NULL);
	if (rc)
	{
		logError();
		return false;
	}
	return true;
}

SqStmt::~SqStmt()
{
	sqlite3_finalize(m_stmt);
}

bool SqStmt::bindInt(int idx, int value)
{
	if (sqlite3_bind_int(m_stmt, idx, value)!=SQLITE_OK)
	{
		logError();
		return false;
	}
	else
	{
		return true;
	}
}

bool SqStmt::bindInt64(int idx, int64_t value)
{
	if (sqlite3_bind_int64(m_stmt, idx, value)!=SQLITE_OK)
	{
		logError();
		return false;
	}
	else
	{
		return true;
	}
}

bool SqStmt::bindText(int idx, const std::string& text)
{
	return bindText(idx, text.c_str());
}

bool SqStmt::bindText(int idx, const char* text)
{
	if (sqlite3_bind_text(m_stmt, idx, text, -1, SQLITE_TRANSIENT)!=SQLITE_OK)
	{
		logError();
		return false;
	}
	else
	{
		return true;
	}
}



bool SqStmt::checkParameters(int numparams)
{
	int numcols = sqlite3_column_count(m_stmt);
	if (numcols!=numparams)
	{
		CZ_LOG(logDefault, Error, L"Invalid number of columns. Statement has %d, callback has %d", numcols, numparams);
		return false;
	}
	return true;
}

bool SqStmt::doStep()
{
	int rc = sqlite3_step(m_stmt);
	if (rc==SQLITE_ROW)
	{
		return true;
	}
	else if (rc==SQLITE_DONE)
	{
		m_done = true;
		return false;
	}
	else
	{
		logError();
		m_done = false;
		return false;
	}
}

void SqStmt::logError()
{
	CZ_LOG(logDefault, Error, L"%s", toUTF16(sqlite3_errmsg(m_db)).c_str());
}


} // namespace cz
