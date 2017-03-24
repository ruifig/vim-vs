/********************************************************************
	CrazyGaze (http://www.crazygaze.com)
	Author : Rui Figueira
	Email  : rui@crazygaze.com
	
	purpose:
	
*********************************************************************/

#pragma once

namespace cz
{

class SqDatabase
{
public:
	SqDatabase() : m_db(nullptr) {}
	~SqDatabase();

	bool open(const char* database, bool create=false);

	operator sqlite3*()
	{
		return m_db;
	}

	operator sqlite3**()
	{
		return &m_db;
	}

	void transaction_begin();
	void transaction_rollback();
	void transaction_commit();

	uint64_t last_insert_rowid();
private:

	sqlite3* m_db;
	SqDatabase(const SqDatabase& other);
	SqDatabase& operator=(const SqDatabase& other);

};

struct SqErrMsg
{
	char* errmsg;
	SqErrMsg() : errmsg(0){}
	~SqErrMsg()
	{
		if (errmsg)
			sqlite3_free(errmsg);
	}

	operator char**()
	{
		return &errmsg;
	}

private:
	SqErrMsg(const SqErrMsg& other);
	SqErrMsg& operator=(const SqErrMsg& other);
};


class SqStmt
{
public:
	SqStmt();
	~SqStmt();
	bool init(sqlite3* db, const char* sql);

	bool bindInt(int idx, int value);
	bool bindInt64(int idx, int64_t value);
	bool bindText(int idx, const std::string& text);
	bool bindText(int idx, const char* text);

private:
	template<typename T>
	T convert(int icol);

	template<>
	const char* convert<const char*>(int iCol)
	{
		const char* s = (const char*)sqlite3_column_text(m_stmt, iCol);
		return s ? s : "";
	}

	template<>
	uint32_t convert<uint32_t>(int iCol)
	{
		return static_cast<uint32_t>(sqlite3_column_int(m_stmt, iCol));
	}

	template<>
	uint8_t convert<uint8_t>(int iCol)
	{
		return static_cast<uint8_t>(sqlite3_column_int(m_stmt, iCol));
	}

	template<>
	uint16_t convert<uint16_t>(int iCol)
	{
		return static_cast<uint16_t>(sqlite3_column_int(m_stmt, iCol));
	}

	template<>
	int convert<int>(int iCol)
	{
		return sqlite3_column_int(m_stmt, iCol);
	}

	template<>
	int64_t convert<int64_t>(int iCol)
	{
		return sqlite3_column_int64(m_stmt, iCol);
	}

	struct AutoReset
	{
		AutoReset(sqlite3_stmt* s) : stm(s){}
		~AutoReset() { sqlite3_reset(stm); }
		sqlite3_stmt* stm;
	};

public:

	bool exec()
	{
		AutoReset _reset(m_stmt);
		bool morework = doStep();
		return !morework; 
	}

	bool exec( const std::function<bool(void)>& callback)
	{
		AutoReset _reset(m_stmt);
		while(doStep())
		{
			if (!checkParameters(0)) return false;
			if (!callback()) break;
		}
		return m_done;
	}

	template<class A0>
	bool exec( const std::function<bool(A0)>& callback)
{
		AutoReset _reset(m_stmt);
		while(doStep())
		{
			if (!checkParameters(1)) return false;
			if (!callback(convert<std::decay<A0>::type>(0))) break;
		}
		return m_done;
	}

	template<class A0, class A1>
	bool exec( const std::function<bool(A0,A1)>& callback)
	{
		AutoReset _reset(m_stmt);
		while(doStep())
		{
			if (!checkParameters(2)) return false;
			if (!callback(convert<std::decay<A0>::type>(0), convert<std::decay<A1>::type>(1))) break;
		}
		return m_done;
	}

	template<class A0, class A1, class A2>
	bool exec( const std::function<bool(A0,A1,A2)>& callback)
	{
		AutoReset _reset(m_stmt);
		while(doStep())
		{
			if (!checkParameters(3)) return false;
			if (!callback(convert<std::decay<A0>::type>(0), convert<std::decay<A1>::type>(1), convert<std::decay<A2>::type>(2))) break;
		}
		return m_done;
	}

	template<class A0, class A1, class A2, class A3>
	bool exec( const std::function<bool(A0,A1,A2,A3)>& callback)
	{
		AutoReset _reset(m_stmt);
		while(doStep())
		{
			if (!checkParameters(4)) return false;
			if (!callback(convert<std::decay<A0>::type>(0), convert<std::decay<A1>::type>(1), convert<std::decay<A2>::type>(2), convert<std::decay<A3>::type>(3))) break;
		}
		return m_done;
	}

	template<class A0, class A1, class A2, class A3, class A4>
	bool exec( const std::function<bool(A0,A1,A2,A3,A4)>& callback)
	{
		AutoReset _reset(m_stmt);
		while(doStep())
		{
			if (!checkParameters(5)) return false;
			if (!callback(convert<std::decay<A0>::type>(0), convert<std::decay<A1>::type>(1), convert<std::decay<A2>::type>(2), convert<std::decay<A3>::type>(3), convert<std::decay<A4>::type>(4))) break;
		}
		return m_done;
	}

	template<class A0, class A1, class A2, class A3, class A4, class A5>
	bool exec( const std::function<bool(A0,A1,A2,A3,A4, A5)>& callback)
	{
		AutoReset _reset(m_stmt);
		while(doStep())
		{
			if (!checkParameters(6)) return false;
			if (!callback(convert<std::decay<A0>::type>(0), convert<std::decay<A1>::type>(1), convert<std::decay<A2>::type>(2), convert<std::decay<A3>::type>(3), convert<std::decay<A4>::type>(4), convert<std::decay<A5>::type>(5))) break;
		}
		return m_done;
	}

	template<class A0, class A1, class A2, class A3, class A4, class A5, class A6>
	bool exec(const std::function<bool(A0,A1,A2,A3,A4,A5,A6)>& callback)
	{
		AutoReset _reset(m_stmt);
		while(doStep())
		{
			if (!checkParameters(7)) return false;
			if (!callback(convert<std::decay<A0>::type>(0), convert<std::decay<A1>::type>(1), convert<std::decay<A2>::type>(2), convert<std::decay<A3>::type>(3), convert<std::decay<A4>::type>(4), convert<std::decay<A5>::type>(5), convert<std::decay<A6>::type>(6))) break;
		}
		return m_done;
	}
	template<class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
	bool exec(const std::function<bool(A0,A1,A2,A3,A4,A5,A6,A7)>& callback)
	{
		AutoReset _reset(m_stmt);
		while(doStep())
		{
			if (!checkParameters(8)) return false;
			if (!callback(convert<std::decay<A0>::type>(0), convert<std::decay<A1>::type>(1), convert<std::decay<A2>::type>(2), convert<std::decay<A3>::type>(3), convert<std::decay<A4>::type>(4), convert<std::decay<A5>::type>(5), convert<std::decay<A6>::type>(6), convert<std::decay<A6>::type>(7))) break;
		}
		return m_done;
	}
private:
	sqlite3* m_db;
	sqlite3_stmt* m_stmt;
	bool m_done;

	bool checkParameters(int numparams);
	bool doStep();
	void logError();
};



class SqTransaction
{
public:
	SqTransaction(SqDatabase &db);
	~SqTransaction();
	void commit();
private:
	// Private and empty, to avoid use
	SqTransaction& operator=(SqTransaction& other);
	SqTransaction(SqTransaction& other);

	SqDatabase &m_db;
	bool m_commited;
};

} // namespace cz
