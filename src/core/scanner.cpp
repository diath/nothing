#include <algorithm>
#include <filesystem>

#include "scanner.hpp"

Scanner::Scanner(const Database *database)
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
	namespace fs = std::filesystem;

	for (auto &&entry: fs::recursive_directory_iterator{path}) {
		if (!running) {
			break;
		}

		(void) entry;
	}
}

Scanner::AddPathResult Scanner::addPath(const std::string &path)
{
	namespace fs = std::filesystem;

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
