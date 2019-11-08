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

#ifndef NOTHING_SCANNER_HPP
#define NOTHING_SCANNER_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class Database;

class Scanner
{
	public:
		enum class AddPathResult
		{
			PathDoesNotExist,
			PathNotDirectory,
			PathAlreadyAdded,
			ParentPathAlreadyAdded,
			Ok,
		};

		Scanner(Database *database);
		~Scanner();

		Scanner(const Scanner &) = delete;
		Scanner(Scanner &&) = delete;

		Scanner &operator =(const Scanner &) = delete;
		Scanner &operator =(Scanner &&) = delete;

		void run();
		void stop();

		void worker();
		void workerTask(const std::string &path);

		AddPathResult addPath(const std::string &path);
		void enqueue(const std::string &path);

		bool removePath(const std::string &path);

		bool isRunning() const;

	private:
		Database *database = nullptr;

		std::atomic<bool> running = false;
		std::vector<std::string> paths{};

		std::vector<std::string> queue{};
		std::mutex mutex{};

		std::thread thread{};
		std::condition_variable cv{};

		std::vector<std::thread> threads{};
};

#endif
