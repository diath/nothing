#ifndef NOTHING_DATABASE_HPP
#define NOTHING_DATABASE_HPP

#include <functional>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include <sqlite3.h>

class Database
{
	public:
		using Entry = std::tuple<std::string, std::string>;
		using QueryCallback = std::function<void(const Entry &)>;

		Database();
		~Database();

		Database(const Database &) = delete;
		Database(Database &&) = delete;

		Database &operator =(const Database &) = delete;
		Database &operator =(Database &&) = delete;

		void addEntry(const Entry &entry);
		void addEntries(const std::vector<Entry> &entries);

		void query(const std::string &pattern, QueryCallback callback);
		void queryRegexp(const std::string &pattern, QueryCallback callback);

	private:
		sqlite3 *handle = nullptr;
		std::mutex mutex{};
};

#endif
