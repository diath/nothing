/*
	Copyright (c) 2019 Kamil Chojnowski Y29udGFjdEBkaWF0aC5uZXQ=

	Permission is hereby granted, free of charge, to any person obtaining a copy of
	this software and associated documentation files (the "Software"), to deal in the
	Software without restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
	the Software, and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
	THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdio>
#include <cstdlib>
#include <regex>

#include "database.hpp"

namespace {

void RegexQuery(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
	if (argc < 2) {
		sqlite3_result_error(ctx, "SQL function regexp() called with invalid arguments.\n", -1);
		return;
	}

	std::string pattern = std::string(reinterpret_cast<const char *>(sqlite3_value_text(argv[0])));
	std::string file = std::string(reinterpret_cast<const char *>(sqlite3_value_text(argv[1])));

	try {
		sqlite3_result_int(ctx, !!std::regex_search(file, std::regex(pattern)));
	} catch (const std::regex_error &) {
		// NOTE: Most commonly, a regex error will occur when someone types unfinished expression, just yield empty result set in that case.
		sqlite3_result_null(ctx);
	}
}

} // namespace <anonymous>

Database::Database()
{
	if (sqlite3_threadsafe() == 0) {
		fprintf(stderr, "[Error] The linked SQLite3 library was not built with the SQLITE_THREADSAFE option.\n");
		std::exit(EXIT_FAILURE);
	}

	sqlite3_config(SQLITE_CONFIG_SERIALIZED);
	sqlite3_initialize();
	if (sqlite3_open_v2(":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
		fprintf(stderr, "[Error] Failed to open the SQLite3 database.\n");
		std::exit(EXIT_FAILURE);
	}

	char *error = nullptr;
	if (sqlite3_exec(handle, "CREATE TABLE files (file TEXT, path TEXT, parent TEXT, size INT, perms INT);", nullptr, nullptr, &error) != SQLITE_OK) {
		fprintf(stderr, "[Error] Failed to create the files table: %s\n", error);
		std::exit(EXIT_FAILURE);
	}

	if (sqlite3_create_function(handle, "regexp", 2, SQLITE_UTF8, nullptr, &RegexQuery, nullptr, nullptr) != SQLITE_OK) {
		fprintf(stderr, "[Error] Failed to register the regexp function\n");
		std::exit(EXIT_FAILURE);
	}
}

Database::~Database()
{
	if (handle != nullptr) {
		stopSearchThread();

		sqlite3_close(handle);
		handle = nullptr;
	}

	sqlite3_shutdown();
}

bool Database::addEntry(const Entry &entry)
{
	std::lock_guard<std::mutex> lock{mutex};

	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare(handle, "INSERT INTO files (file, path, parent, size, perms) VALUES (?, ?, ?, ?, ?);", -1, &stmt, nullptr) != SQLITE_OK) {
		return false;
	}

	const auto &[name, path, parent, size, perms] = entry;

	if (sqlite3_bind_text(stmt, 1, name.c_str(), -1, nullptr) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return false;
	}

	if (sqlite3_bind_text(stmt, 2, path.c_str(), -1, nullptr) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return false;
	}

	if (sqlite3_bind_text(stmt, 3, parent.c_str(), -1, nullptr) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return false;
	}

	if (sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(size)) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return false;
	}

	if (sqlite3_bind_int(stmt, 5, static_cast<int>(perms)) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return false;
	}

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return true;
}

bool Database::addEntries(const std::vector<Entry> &entries)
{
	std::lock_guard<std::mutex> lock{mutex};

	sqlite3_exec(handle, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare(handle, "INSERT INTO files (file, path, parent, size, perms) VALUES (?, ?, ?, ?, ?);", -1, &stmt, nullptr) != SQLITE_OK) {
		return false;
	}

	auto cleanup = [this, stmt] () {
		sqlite3_exec(handle, "ROLLBACK TRANSACTION", nullptr, nullptr, nullptr);
		sqlite3_finalize(stmt);
	};

	for (auto &&entry: entries) {
		const auto &[name, path, parent, size, perms] = entry;
		if (sqlite3_bind_text(stmt, 1, name.c_str(), -1, nullptr) != SQLITE_OK) {
			cleanup();
			return false;
		}

		if (sqlite3_bind_text(stmt, 2, path.c_str(), -1, nullptr) != SQLITE_OK) {
			cleanup();
			return false;
		}

		if (sqlite3_bind_text(stmt, 3, parent.c_str(), -1, nullptr) != SQLITE_OK) {
			cleanup();
			return false;
		}

		if (sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(size)) != SQLITE_OK) {
			cleanup();
			return false;
		}

		if (sqlite3_bind_int(stmt, 5, static_cast<int>(perms)) != SQLITE_OK) {
			cleanup();
			return false;
		}

		sqlite3_step(stmt);
		sqlite3_reset(stmt);
	}

	sqlite3_finalize(stmt);
	sqlite3_exec(handle, "END TRANSACTION", nullptr, nullptr, nullptr);

	return true;
}

bool Database::removeEntry(const std::string &name, const std::string &path)
{
	std::lock_guard<std::mutex> lock{mutex};

	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare(handle, "DELETE FROM files WHERE file = ? AND path = ?;", -1, &stmt, nullptr) != SQLITE_OK) {
		return false;
	}

	if (sqlite3_bind_text(stmt, 1, name.c_str(), -1, nullptr) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return false;
	}

	if (sqlite3_bind_text(stmt, 2, path.c_str(), -1, nullptr) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return false;
	}

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return true;
}

bool Database::removeEntries(const std::string &parent)
{
	std::lock_guard<std::mutex> lock{mutex};

	sqlite3_exec(handle, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare(handle, "DELETE FROM files WHERE parent = ?", -1, &stmt, nullptr) != SQLITE_OK) {
		return false;
	}

	if (sqlite3_bind_text(stmt, 1, parent.c_str(), -1, nullptr) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return false;
	}

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	sqlite3_exec(handle, "END TRANSACTION", nullptr, nullptr, nullptr);

	return true;
}

bool Database::removeEntriesByPath(const std::string &path)
{
	std::lock_guard<std::mutex> lock{mutex};

	sqlite3_exec(handle, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare(handle, "DELETE FROM files WHERE parent = ?", -1, &stmt, nullptr) != SQLITE_OK) {
		return false;
	}

	if (sqlite3_bind_text(stmt, 1, path.c_str(), -1, nullptr) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return false;
	}

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	sqlite3_exec(handle, "END TRANSACTION", nullptr, nullptr, nullptr);

	return true;
}

void Database::query(const std::string &pattern, const bool regexp, QueryCallback callback, QueryDoneCallback doneCallback/* = {} */)
{
	if (regexp) {
		queryRegexp(pattern, callback, doneCallback);
	} else {
		queryLike(pattern, callback, doneCallback);
	}
}

void Database::queryLike(const std::string &pattern, const QueryCallback &callback, const QueryDoneCallback &doneCallback)
{
	queryInternal("SELECT file, path, parent, size, perms FROM files WHERE file LIKE ?;", "%" + pattern + "%", callback, doneCallback);
}

void Database::queryRegexp(const std::string &pattern, const QueryCallback &callback, const QueryDoneCallback &doneCallback)
{
	queryInternal("SELECT file, path, parent, size, perms FROM files WHERE file REGEXP(?);", pattern, callback, doneCallback);
}

void Database::queryInternal(const std::string &query, const std::string &pattern, const QueryCallback &callback, const QueryDoneCallback &doneCallback)
{
	static std::atomic<std::size_t> QueryIndex = 0;
	++QueryIndex;

	stopSearchThread();

	searchStopped = false;
	searchThread = std::thread([this, query, pattern, callback, doneCallback] () {
		sqlite3_stmt *stmt = nullptr;
		if (sqlite3_prepare(handle, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
			fprintf(stderr, "[Error] Failed to prepare query statement: %s.\n", sqlite3_errmsg(handle));
			return;
		}

		if (sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, nullptr) != SQLITE_OK) {
			fprintf(stderr, "[Error] Failed to bind query parameter: %s\n", sqlite3_errmsg(handle));
			sqlite3_finalize(stmt);
			return;
		}

		int result = SQLITE_OK;
		while ((result = sqlite3_step(stmt)) != SQLITE_DONE && !searchStopped) {
			if (result != SQLITE_ROW) {
				fprintf(stderr, "[Error] Failed to fetch a query result: %s\n", sqlite3_errmsg(handle));
				sqlite3_finalize(stmt);
				return;
			}

			Entry entry{};
			auto &[file, path, parent, size, perms] = entry;
			file = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
			path = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
			parent = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
			size = static_cast<std::uintmax_t>(sqlite3_column_int64(stmt, 3));
			perms = static_cast<std::filesystem::perms>(sqlite3_column_int(stmt, 4));

			callback(QueryIndex, entry);
		}

		sqlite3_finalize(stmt);

		if (doneCallback) {
			doneCallback();
		}
	});
}

void Database::stopSearchThread()
{
	searchStopped = true;

	if (searchThread.joinable()) {
		searchThread.join();
	}
}
