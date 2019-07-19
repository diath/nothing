#include <cstdio>

#include "database.hpp"

Database::Database()
{
	if (sqlite3_threadsafe() == 0) {
		printf("[Warning] The linked SQLite3 library was not built with the SQLITE_THREADSAFE option.\n");
	}

	sqlite3_initialize();
	if (sqlite3_open(":memory:", &handle) != SQLITE_OK) {
		fprintf(stderr, "[Error] Failed to open the SQLite3 database.\n");
		// TODO: Terminate?
	}

	char *error_message = nullptr;
	if (sqlite3_exec(handle, "CREATE TABLE files (file TEXT, path TEXT);", nullptr, nullptr, &error_message) != SQLITE_OK) {
		fprintf(stderr, "[Error] Failed to create the files table: %s\n", error_message);
		// TODO: Terminate?
	}
}

Database::~Database()
{
	if (handle != nullptr) {
		sqlite3_close(handle);
		handle = nullptr;
	}

	sqlite3_shutdown();
}

void Database::addEntry(const Entry &entry)
{
	std::unique_lock<std::mutex> lock{mutex};

	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare(handle, "INSERT INTO files (file, path) VALUES (?, ?);", -1, &stmt, nullptr) != SQLITE_OK) {
		// TODO: Handle gracefully?
		return;
	}

	const auto &[name, path] = entry;

	if (sqlite3_bind_text(stmt, 1, name.c_str(), -1, nullptr) != SQLITE_OK) {
		// TODO: Handle gracefully?
		return;
	}

	if (sqlite3_bind_text(stmt, 2, path.c_str(), -1, nullptr) != SQLITE_OK) {
		// TODO: Handle gracefully?
		return;
	}

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

void Database::addEntries(const std::vector<Entry> &entries)
{
	std::unique_lock<std::mutex> lock{mutex};

	sqlite3_exec(handle, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare(handle, "INSERT INTO files (file, path) VALUES (?, ?);", -1, &stmt, nullptr) != SQLITE_OK) {
		// TODO: Handle gracefully?
		return;
	}

	for (auto &&entry: entries) {
		const auto &[name, path] = entry;

		if (sqlite3_bind_text(stmt, 1, name.c_str(), -1, nullptr) != SQLITE_OK) {
			// TODO: Handle gracefully?
			return;
		}

		if (sqlite3_bind_text(stmt, 2, path.c_str(), -1, nullptr) != SQLITE_OK) {
			// TODO: Handle gracefully?
			return;
		}

		sqlite3_step(stmt);
		sqlite3_reset(stmt);
	}

	sqlite3_finalize(stmt);
	sqlite3_exec(handle, "END TRANSACTION", nullptr, nullptr, nullptr);
}
