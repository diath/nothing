#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

struct Entry
{
	std::string name;
};

struct Database
{
};

struct Scanner
{
	Scanner(const Database &database)
	: database{database}
	{
	}

	~Scanner()
	{
		stop();
	}

	void run()
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

	void stop()
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

		queue.clear();
	}

	void worker()
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

	void workerTask(const std::string &path)
	{
		for (auto &&entry: fs::recursive_directory_iterator{path}) {
			if (!running) {
				break;
			}

			(void) entry;
		}
	}

	bool addPath(const std::string &path)
	{
		if (!fs::exists(path)) {
			return false;
		}

		if (!fs::is_directory(path)) {
			return false;
		}

		if (std::find(paths.begin(), paths.end(), path) != paths.end()) {
			return false;
		}

		paths.push_back(path);

		if (running) {
			enqueue(path);
		}

		return true;
	}

	void enqueue(const std::string &path)
	{
		std::lock_guard<std::mutex> lock{mutex};

		if (std::find(queue.begin(), queue.end(), path) != queue.end()) {
			return;
		}

		queue.push_back(path);
		cv.notify_one();
	}

	bool isRunning() const {
		return running;
	}

	private:
		const Database &database;

		std::atomic<bool> running = false;
		std::vector<std::string> paths{};

		std::vector<std::string> queue{};
		std::mutex mutex{};

		std::thread thread{};
		std::condition_variable cv{};

		std::vector<std::thread> threads{};
};

int main(int argc, char **argv)
{
	Database db{};
	Scanner scanner{db};
	if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			if (!scanner.addPath(argv[i])) {
				fprintf(stderr, "Failed to add the path: %s\n", argv[i]);
			} else {
				printf("Added %s\n", argv[i]);
			}
		}
	}

	scanner.run();
	std::string line{};
	if (scanner.isRunning()) {
		std::cin.ignore();
		std::getline(std::cin, line);

		if (line == "stop") {
			scanner.stop();
		} else {
			scanner.addPath(line);
		}

		std::cin.ignore();
	}

	return 0;
}
