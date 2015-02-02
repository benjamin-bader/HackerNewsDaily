#include "pch.h"
#include "Database.h"

#include <cassert>
#include <codecvt>
#include <iostream>
#include <locale>
#include <mutex>
#include <string>

using std::codecvt_utf8;
using std::once_flag;
using std::string;
using std::wstring_convert;

using namespace Amplitude;

// SQLite requires SQL to be encoded as UTF-8; as these are all ASCII, we're good.  Just FYI.
static const char * const kCreateTable = "CREATE TABLE IF NOT EXISTS events (id INTEGER PRIMARY KEY AUTOINCREMENT, event TEXT);";
static const char * const kInsertEvent = "INSERT INTO events (event) VALUES (?);";
static const char * const kGetEventsSince = "SELECT id, event FROM events ORDER BY id ASC;";
static const char * const kGetEventsSinceBounded = "SELECT id, event FROM events WHERE id < ? ORDER BY id ASC;";
static const char * const kGetEventsSinceWithLimit = "SELECT id, event FROM events ORDER BY id ASC LIMIT ?;";
static const char * const kGetEventsSinceBoundedeWithLimit = "SELECT id, event FROM events WHERE id < ? ORDER BY id ASC LIMIT ?;";
static const char * const KGetEventCount = "SELECT COUNT(id) FROM events;";
static const char * const kGetNthEventId = "SELECT id FROM events LIMIT 1 OFFSET (? - 1);";
static const char * const kDeleteEventsBefore = "DELETE FROM events WHERE id <= ?;";
static const char * const kDeleteSingleEvent = "DELETE FROM events WHERE id = ?;";

// Declared as extern in <sqlite.h>, need to define it here
char * sqlite3_temp_directory;

static string WideToMulti(String ^path)
{
	typedef codecvt_utf8<wchar_t> converter;

	wstring_convert<converter, wchar_t> c;
	return c.to_bytes(path->Data());
}

static Platform::String^ MultiToWide(const char *chars)
{
	typedef codecvt_utf8<wchar_t> converter;

	wstring_convert<converter, wchar_t> c;
	return ref new Platform::String(c.from_bytes(chars).data());
}

//
// IHasDatabase
//
// Base class that holds a SQLite database pointer and routines
// that are common to Database and Statement that require such
// a pointer.

class IHasDatabase
{
public:
	IHasDatabase(IHasDatabase const&) = delete;
	IHasDatabase(IHasDatabase&&) = delete;
	IHasDatabase& operator=(IHasDatabase const&) = delete;

protected:
	sqlite3 *db_;

	IHasDatabase() {}

	void Check(int rc, int expected = SQLITE_OK);
};

void IHasDatabase::Check(int rc, int expected)
{
	if (rc != expected)
	{
		auto msg = sqlite3_errmsg(db_);
		throw ref new Platform::FailureException(MultiToWide(msg));
	}
}


//
// Statement
//
// A lightweight RAII wrapper around sqlite3 prepared statements.

class Statement : protected IHasDatabase
{
public:
	Statement(sqlite3 *db, const char *sql);
	~Statement();

	void Bind(int index, int value);
	void Bind(int index, int64 value);
	void Bind(int index, const std::string &value);

	void Reset();

	bool Step();
	int Exec();

	int GetColumnCount();

	int IntColumn(int index);
	int64 Int64Column(int index);
	String^ TextColumn(int index);
private:
	sqlite3_stmt *stmt;
};

Statement::Statement(sqlite3 *db, const char * sql)
{
	db_ = db;

	auto rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
	Check(rc, SQLITE_OK);
}

Statement::~Statement()
{
	auto rc = sqlite3_finalize(stmt);
#ifdef DEBUG
	if (rc != SQLITE_OK)
	{
		std::string msg(sqlite3_errmsg(db_));
		std::cout << msg << std::endl;
	}
#endif
}

void
Statement::Bind(int index, int value)
{
	auto rc = sqlite3_bind_int(stmt, index, value);
	Check(rc, SQLITE_OK);
}

void
Statement::Bind(int index, int64 value)
{
	auto rc = sqlite3_bind_int64(stmt, index, value);
	Check(rc, SQLITE_OK);
}

void
Statement::Bind(int index, const std::string  &value)
{
	auto rc = sqlite3_bind_text(stmt, index, value.data(), value.length(), SQLITE_STATIC);
	Check(rc, SQLITE_OK);
}

void
Statement::Reset()
{
	auto rc = sqlite3_reset(stmt);
	Check(rc, SQLITE_OK);
}

bool
Statement::Step()
{
	auto rc = sqlite3_step(stmt);

	if (rc == SQLITE_ROW)
	{
		return true;
	}

	if (rc == SQLITE_DONE)
	{
		return false;
	}

	Check(rc, -1); // force an exception
	throw ref new Platform::FailureException(L"Impossible");
}

int
Statement::Exec()
{
	auto rc = sqlite3_step(stmt);

	Check(rc, SQLITE_DONE);

	return sqlite3_changes(db_);
}

int
Statement::GetColumnCount()
{
	return sqlite3_column_count(stmt);
}

int
Statement::IntColumn(int index)
{
	return sqlite3_column_int(stmt, index);
}

int64
Statement::Int64Column(int index)
{
	return sqlite3_column_int64(stmt, index);
}

String^
Statement::TextColumn(int index)
{
	auto chars = sqlite3_column_text(stmt, index);
	auto signedChars = reinterpret_cast<const char *>(chars);
	return MultiToWide(signedChars);
}


//
// Database::Impl
//
// Private implementation details of the Amplitude SQLite database.

class Database::Impl : protected IHasDatabase
{
public:
	Impl(Platform::String ^path);
	~Impl();

	int64 AddEvent(JsonObject ^eventObj);

	int64 GetEventCount();

	pair<int64, JsonArray^> GetEventsSince(int64 eventId, int limit);
	int64 GetNthEventId(int n);

	int RemoveEvents(int64 maxId);
	int RemoveSingleEvent(int64 eventId);
};

Database::Impl::Impl(Platform::String ^path)
{
	static std::once_flag initFlag;
	std::call_once(initFlag, []
	{
		// Per http://www.sqlite.org/c3ref/temp_directory.html, the temp directory
		// must be set *before* opening any database connections at all.
		auto rtPath = Windows::Storage::ApplicationData::Current->TemporaryFolder->Path;
		auto path = WideToMulti(rtPath);
		auto tmpDir = sqlite3_mprintf("%s", path.c_str());
		sqlite3_temp_directory = tmpDir;
	});

	auto narrowPath = WideToMulti(path);
	auto rc = sqlite3_open(narrowPath.data(), &db_);
	Check(rc, SQLITE_OK);

	// Make sure the events table exists
	Statement stmt(db_, kCreateTable);
	stmt.Exec();
}

Database::Impl::~Impl()
{
	auto rc = sqlite3_close(db_);
#ifdef DEBUG
	if (rc != 0)
	{
		std::string msg(sqlite3_errmsg(db_));
		std::cerr << msg << std::endl;
	}
#endif
}

int64
Database::Impl::AddEvent(JsonObject ^eventObj)
{
	Statement stmt(db_, kInsertEvent);

	auto str = WideToMulti(eventObj->Stringify());
	stmt.Bind(1, str);

	auto rows = stmt.Exec();

	return sqlite3_last_insert_rowid(db_);
}

enum querytype {
	bounded_and_limited,
	bounded,
	limited,
	all
};

pair<int64, JsonArray^>
Database::Impl::GetEventsSince(int64 eventId, int limit = 0)
{
	const char *query;
	querytype state;
	if (eventId >= 0 && limit > 0)
	{
		state = bounded_and_limited;
		query = kGetEventsSinceBoundedeWithLimit;
	}
	else if (eventId >= 0)
	{
		state = bounded;
		query = kGetEventsSinceBounded;
	}
	else if (limit > 0)
	{
		state = limited;
		query = kGetEventsSinceWithLimit;
	}
	else
	{
		state = all;
		query = kGetEventsSince;
	}

	Statement stmt(db_, query);

	switch (state)
	{
	case bounded_and_limited:
		stmt.Bind(1, eventId);
		stmt.Bind(2, limit);
		break;
	case bounded:
		stmt.Bind(1, eventId);
		break;
	case limited:
		stmt.Bind(1, limit);
		break;
	}

	auto events = ref new JsonArray();
	auto maxId = -1LL;
	while (stmt.Step())
	{
		auto id = stmt.Int64Column(0);
		auto text = stmt.TextColumn(1);

		if (id > maxId)
		{
			maxId = id;
		}

		auto obj = Windows::Data::Json::JsonObject::Parse(text);

		// nowarn because the range of integers representable as a double
		// is roughly 2^52, and we are almost certain to never have that
		// many event IDs.
#pragma warning (push)
#pragma warning (disable: 4244)
		obj->Insert("event_id", Windows::Data::Json::JsonValue::CreateNumberValue(id));
#pragma warning (pop)

		events->Append(obj);
	}

	return std::make_pair(maxId, events);
}

int64
Database::Impl::GetEventCount()
{
	Statement stmt(db_, KGetEventCount);

	auto result = 0LL;
	if (stmt.Step())
	{
		result = stmt.Int64Column(0);
	}
	return result;
}

int64
Database::Impl::GetNthEventId(int n)
{
	Statement stmt(db_, kGetNthEventId);
	stmt.Bind(1, n);

	auto result = -1LL;
	if (stmt.Step())
	{
		result = stmt.Int64Column(0);
	}

	return result;
}

int
Database::Impl::RemoveEvents(int64 maxId)
{
	Statement stmt(db_, kDeleteEventsBefore);
	stmt.Bind(1, maxId);

	return stmt.Exec();
}

int
Database::Impl::RemoveSingleEvent(int64 eventId)
{
	Statement stmt(db_, kDeleteSingleEvent);
	stmt.Bind(1, eventId);

	return stmt.Exec();
}


// Database
//
// 


Database::Database(String ^path) : impl(std::make_unique<Impl>(path))
{
}

Database::~Database()
{
}

int64
Database::AddEvent(JsonObject ^eventObj)
{
	return impl->AddEvent(eventObj);
}

pair<int64, JsonArray^>
Database::GetEventsSince(int64 eventId, int limit = 0)
{
	return impl->GetEventsSince(eventId, limit);
}

int64
Database::GetNthEventId(int n)
{
	return impl->GetNthEventId(n);
}

int64
Database::GetEventCount()
{
	return impl->GetEventCount();
}

int
Database::RemoveEvents(int64 maxId)
{
	return impl->RemoveEvents(maxId);
}

int
Database::RemoveSingleEvent(int64 eventId)
{
	return impl->RemoveSingleEvent(eventId);
}