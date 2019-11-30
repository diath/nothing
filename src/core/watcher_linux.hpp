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

#ifndef NOTHING_WATCHER_LINUX_HPP
#define NOTHING_WATCHER_LINUX_HPP

#include <atomic>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class Database;

class Watcher
{
	using Descriptor = std::tuple<int, std::string>;
	using Descriptors = std::vector<Descriptor>;

	public:
		Watcher(Database *database);
		~Watcher();

		Watcher(const Watcher &) = delete;
		Watcher(Watcher &&) = delete;

		Watcher &operator =(const Watcher &) = delete;
		Watcher &operator =(Watcher &&) = delete;

		void run();
		void stop();

		bool watch(const std::string &path);
		bool unwatch(const std::string &path);

	private:
		Database *database = nullptr;

		std::atomic<bool> running = false;
		std::thread thread = {};

		int fd = -1;

		/*
			The inotify_event structure only returns the watch descriptor and the file and folder name but not their full path.
			So we create some manual mapping to keep track of things.

			The descriptors vector holds all top parent folders.
			The childDescriptors map maps the parent paths to child folders, used for unwatching child folders when a parent is unwatched.
			The folders map maps watch descriptors to full folder paths.
			The parents map maps watch descriptors to top parent paths.

			This could likely be cleaned up.
		*/
		Descriptors descriptors = {};
		std::unordered_map<std::string, Descriptors> childDescriptors = {};
		std::unordered_map<int, std::string> folders = {};
		std::unordered_map<int, std::string> parents = {};

		void worker();

		bool watchInternal(const std::string &parent, const std::string &path);

		void onEvent();
		void onFileCreated(int wd, const std::string &path);
		void onFileDeleted(int wd, const std::string &path);
		void onDirectoryCreated(int wd, const std::string &path);
		void onDirectoryDeleted(int wd, const std::string &path);
};

#endif // NOTHING_WATCHER_LINUX_HPP
