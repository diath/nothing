#ifndef NOTHING_DATABASE_HPP
#define NOTHING_DATABASE_HPP

#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include <sqlite3.h>

class Database
{
	public:
		using Entry = std::tuple<std::string, std::string>;

		Database();
		~Database();

		Database(const Database &) = delete;
		Database(Database &&) = delete;

		Database &operator =(const Database &) = delete;
		Database &operator =(Database &&) = delete;

		void addEntry(const Entry &entry);
		void addEntries(const std::vector<Entry> &entries);

	private:
		sqlite3 *handle = nullptr;
		std::mutex mutex{};
};

#endif
