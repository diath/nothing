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
			Ok,
		};

		Scanner(const Database *database);
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

		bool isRunning() const;

	private:
		const Database *database = nullptr;

		std::atomic<bool> running = false;
		std::vector<std::string> paths{};

		std::vector<std::string> queue{};
		std::mutex mutex{};

		std::thread thread{};
		std::condition_variable cv{};

		std::vector<std::thread> threads{};
};

#endif
