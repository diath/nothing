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

#include <algorithm>
#include <filesystem>

#include <chrono>

#include "scanner.hpp"
#include "database.hpp"

namespace fs = std::filesystem;

Scanner::Scanner(Database *database)
: database{database}
{
}

Scanner::~Scanner()
{
	stop();
}

void Scanner::run()
{
	if (running) {
		return;
	}

	for (auto &&path: paths) {
		enqueue(path);
	}

	running = true;
	thread = std::thread([this] () {
		worker();
	});
}

void Scanner::stop()
{
	if (!running) {
		return;
	}

	running = false;

	queue.clear();
	cv.notify_one();

	if (thread.joinable()) {
		thread.join();
	}

	for (auto &&thread: threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
	threads.clear();
}

void Scanner::worker()
{
	std::unique_lock<std::mutex> lock{mutex};
	while (running) {
		cv.wait(lock, [this] {
			return !queue.empty() || !running;
		});

		if (!running) {
			break;
		}

		for (auto &&path: queue) {
			threads.emplace_back([this, path] () {
				workerTask(path);
			});
		}

		queue.clear();
	}
}

void Scanner::workerTask(const std::string &path)
{
	constexpr auto BatchSize = 32 * 1024u;

	std::vector<Database::Entry> entries{};
	entries.reserve(BatchSize);

	std::size_t counter = 0;
	for (auto &&entry: fs::recursive_directory_iterator{path, fs::directory_options::skip_permission_denied}) {
		if (!running) {
			break;
		}

		// TODO: This should be interrupted when a path has been removed.

		if (entry.is_directory()) {
			continue;
		}

		std::error_code ec{};

		auto &filePath = entry.path();
		auto status = entry.status();

		entries.emplace_back(
			filePath.filename().string(),
			filePath.parent_path().string(),
			path,
			entry.file_size(ec),
			status.permissions()
		);

		if (++counter == BatchSize) {
			database->addEntries(entries);
			entries.clear();
			counter = 0;
		}
	}
}

Scanner::AddPathResult Scanner::addPath(const std::string &path)
{
	if (!fs::exists(path)) {
		return AddPathResult::PathDoesNotExist;
	}

	if (!fs::is_directory(path)) {
		return AddPathResult::PathNotDirectory;
	}

	if (std::find(paths.begin(), paths.end(), path) != paths.end()) {
		return AddPathResult::PathAlreadyAdded;
	}

	paths.push_back(path);

	if (running) {
		enqueue(path);
	}

	return AddPathResult::Ok;
}

void Scanner::enqueue(const std::string &path)
{
	std::lock_guard<std::mutex> lock{mutex};

	if (std::find(queue.begin(), queue.end(), path) != queue.end()) {
		return;
	}

	queue.push_back(path);
	cv.notify_one();
}

bool Scanner::removePath(const std::string &path)
{
	if (auto it = std::find(paths.begin(), paths.end(), path); it != paths.end()) {
		paths.erase(it);
	} else {
		return false;
	}

	std::lock_guard<std::mutex> lock{mutex};
	if (auto it = std::find(queue.begin(), queue.end(), path); it != queue.end()) {
		queue.erase(it);
	}

	database->removeEntries(path);
	return true;
}

bool Scanner::isRunning() const
{
	return running;
}
