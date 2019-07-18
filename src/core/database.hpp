#ifndef NOTHING_DATABASE_HPP
#define NOTHING_DATABASE_HPP

#include <mutex>
#include <string>

#include <sqlite3.h>

class Database
{
	public:
		Database();
		~Database();

		Database(const Database &) = delete;
		Database(Database &&) = delete;

		Database &operator =(const Database &) = delete;
		Database &operator =(Database &&) = delete;

		void addEntry(const std::string &file, const std::string &path);

	private:
		sqlite3 *handle = nullptr;
		std::mutex mutex{};
};

#endif
