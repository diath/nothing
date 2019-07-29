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

#ifndef NOTHING_DATABASE_HPP
#define NOTHING_DATABASE_HPP

#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include <sqlite3.h>

class Database
{
	public:
		using Entry = std::tuple<std::string, std::string, std::uintmax_t, std::filesystem::perms>;
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
