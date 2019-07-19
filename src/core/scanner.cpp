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
	for (auto &&entry: fs::recursive_directory_iterator{path}) {
		if (!running) {
			break;
		}

		if (entry.is_directory()) {
			continue;
		}

		auto &path = entry.path();
		entries.emplace_back(path.filename().string(), path.parent_path().string());

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

bool Scanner::isRunning() const
{
	return running;
}
