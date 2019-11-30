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

#include <poll.h>
#include <unistd.h>
#include <sys/inotify.h>

#include <algorithm>
#include <filesystem>

#include "watcher_linux.hpp"
#include "database.hpp"

namespace fs = std::filesystem;

Watcher::Watcher(Database *database)
: database{database}
{
}

Watcher::~Watcher()
{
	stop();
}

void Watcher::run()
{
	fd = inotify_init1(IN_NONBLOCK);
	if (fd == -1) {
		std::fprintf(stderr, "[Watcher] run(): Failed to initialize inotify (%d)\n", errno);
		std::exit(EXIT_FAILURE);
	}

	running = true;
	thread = std::thread([this] () {
		worker();
	});
}

void Watcher::stop()
{
	if (!running) {
		return;
	}

	running = false;

	if (fd != -1) {
		close(fd);
		fd = -1;
	}

	for (const auto &descriptor: descriptors) {
		const auto &[_, path] = descriptor;
		unwatch(path);
	}

	if (thread.joinable()) {
		thread.join();
	}
}

void Watcher::worker()
{
	pollfd fds[1] = {};
	fds[0].fd = fd;
	fds[0].events = POLLIN;

	while (running) {
		int res = poll(fds, 1, 100);
		if (!running) {
			break;
		}

		if (res == -1) {
			if (errno == EINTR) {
				continue;
			}

			std::fprintf(stderr, "[Watcher] worker(): Failed to poll the watcher file descriptor\n");
			std::exit(EXIT_FAILURE);
		}

		if (res > 0 && (fds[0].revents & POLLIN)) {
			onEvent();
		}
	}
}

bool Watcher::watchInternal(const std::string &parent, const std::string &path)
{
	auto res = inotify_add_watch(
		fd, path.c_str(),
		IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO
	);
	if (res == -1) {
		std::fprintf(stderr, "[Watcher] watchInternal(): Call to inotify_add_watch() failed for %s in %s (error: %d)\n",
			path.c_str(), parent.c_str(), errno
		);
		return false;
	}

	#if not defined(NDEBUG)
		std::printf("[Watcher] watchInternal(): Watching %s in %s (wd: %d)\n", path.c_str(), parent.c_str(), res);
	#endif

	childDescriptors[parent].emplace_back(res, path);
	folders[res] = path;
	parents[res] = parent;

	return true;
}

bool Watcher::watch(const std::string &path)
{
	auto it = std::find_if(descriptors.begin(), descriptors.end(), [&path] (auto &&descriptor) {
		const auto &[_, descriptorPath] = descriptor;
		return descriptorPath == path;
	});
	if (it != descriptors.end()) {
		std::printf("[Watcher] watch(): Path %s is already being watched, ignoring\n", path.c_str());
		return true;
	}

	auto res = inotify_add_watch(
		fd, path.c_str(),
		IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO
	);
	if (res == -1) {
		std::fprintf(stderr, "[Watcher] watch(): Call to inotify_add_watch() failed for %s (error: %d)\n",
			path.c_str(), errno
		);
		return false;
	}

	try {
		for (auto &&entry: fs::recursive_directory_iterator{path, fs::directory_options::skip_permission_denied}) {
			if (!entry.is_directory()) {
				continue;
			}

			if (!watchInternal(path, entry.path())) {
				childDescriptors.erase(path);
				unwatch(path);
				return false;
			}
		}
	} catch (const std::filesystem::filesystem_error &e) {
		unwatch(path);
		std::fprintf(stderr, "[Watcher] watch(): Filesystem error: %s\n", e.what());
		return false;
	}

	#if not defined(NDEBUG)
		std::printf("[Watcher] watch(): Watching %s (wd: %d)\n", path.c_str(), res);
	#endif

	descriptors.emplace_back(res, path);
	folders[res] = path;
	parents[res] = path;

	return true;
}

bool Watcher::unwatch(const std::string &path)
{
	// NOTE: This should only be called on top parent paths.
	auto it = std::find_if(descriptors.begin(), descriptors.end(), [&path] (auto &&descriptor) {
		const auto &[_, descriptorPath] = descriptor;
		return descriptorPath == path;
	});

	if (it == descriptors.end()) {
		std::fprintf(stderr, "[Watcher] unwatch(): Failed to unwatch %s (path not in descriptor list)\n", path.c_str());
		return false;
	}

	if (childDescriptors.count(path) > 0) {
		for (const auto &child: childDescriptors[path]) {
			const auto &[wd, path] = child;

			if (inotify_rm_watch(fd, wd) == -1) {
			}

			folders.erase(wd);
			parents.erase(wd);
		}

		childDescriptors.erase(path);
	}

	const auto &[wd, _] = *it;

	descriptors.erase(it);
	folders.erase(wd);
	parents.erase(wd);

	auto res = inotify_rm_watch(fd, wd);
	if (res == -1) {
		std::fprintf(stderr, "[Watcher] unwatch(): Call to inotify_rm_watch() failed for %s (error: %d)\n",
			path.c_str(), errno
		);
	}

	return res == 0;
}

void Watcher::onEvent()
{
	/*
		http://man7.org/linux/man-pages/man7/inotify.7.html

		Some systems cannot read integer variables if they are not
		properly aligned. On other systems, incorrect alignment may
		decrease performance. Hence, the buffer used for reading from
		the inotify file descriptor should have the same alignment as
		struct inotify_event.
	*/
	char buffer[4096] __attribute__ ((aligned(__alignof__(inotify_event)))) = {};
	auto step = sizeof(inotify_event);

	while (true) {
		int size = read(fd, buffer, sizeof(buffer));
		if (size == -1 && errno != EAGAIN) {
			std::fprintf(stderr, "[Error] onEvent(): Failed to read from inotify file descriptor (%d)\n", errno);
			std::exit(EXIT_FAILURE);
		}

		if (size <= 0) {
			break;
		}

		int index = 0;
		while (index < size) {
			auto event = reinterpret_cast<inotify_event *>(&buffer[index]);
			if (event->len > 0) {
				if ((event->mask & IN_CREATE) || (event->mask & IN_MOVED_TO)) {
					if ((event->mask & IN_ISDIR)) {
						onDirectoryCreated(event->wd, event->name);
					} else {
						onFileCreated(event->wd, event->name);
					}
				} else if ((event->mask & IN_DELETE) || (event->mask & IN_MOVED_FROM)) {
					if ((event->mask & IN_ISDIR)) {
						onDirectoryDeleted(event->wd, event->name);
					} else {
						onFileDeleted(event->wd, event->name);
					}
				}
			}

			index += step;
			index += event->len;
		}
	}
}

void Watcher::onFileCreated(int wd, const std::string &name)
{
	#if not defined(NDEBUG)
		std::printf("[Watcher] onFileCreated(): File %s created (wd: %d)\n", name.c_str(), wd);
	#endif

	auto it = folders.find(wd);
	if (it == folders.end()) {
		std::fprintf(stderr, "[Watcher] onFileCreated(): Failed to find the full path for %s (wd: %d)\n",
			name.c_str(), wd
		);
		return;
	}

	auto itParent = parents.find(wd);
	if (it == parents.end()) {
		std::fprintf(stderr, "[Watcher] onFileCreated(): Failed to find the top parent for %s (wd: %d)\n",
			name.c_str(), wd
		);
		return;
	}

	#if not defined(NDEBUG)
		std::printf("[Watcher] onFileCreated(): File %s created in %s (parent: %s, wd: %d)\n",
			name.c_str(), it->second.c_str(), itParent->second.c_str(), wd
		);
	#endif

	Database::Entry entry{};
	std::error_code ec{};

	auto fsPath = fs::path(it->second) / name;
	auto &[file, path, parent, size, perms] = entry;

	file = name;
	path = it->second;
	parent = itParent->second;
	size = fs::file_size(fsPath, ec);
	perms = fs::status(fsPath, ec).permissions();

	if (!database->addEntry(entry)) {
		std::fprintf(stderr, "[Watcher] onFileCreated(): Failed to add the entry to the database (%s)\n",
			fsPath.c_str()
		);
	}
}

void Watcher::onFileDeleted(int wd, const std::string &name)
{
	#if not defined(NDEBUG)
		std::printf("[Watcher] onFileDeleted(): File %s deleted (wd: %d)\n", name.c_str(), wd);
	#endif

	auto it = folders.find(wd);
	if (it == folders.end()) {
		std::fprintf(stderr, "[Watcher] onFileDeteled(): Failed to find the full path for %s (wd: %d)",
			name.c_str(), wd
		);
		return;
	}

	#if not defined(NDEBUG)
		std::printf("[Watcher] onFileDeleted(): File %s deleted in %s (wd: %d)\n",
			name.c_str(), it->second.c_str(), wd
		);
	#endif

	if (!database->removeEntry(name, it->second)) {
		std::fprintf(stderr, "[Watcher]: onFileDeleted: Failed to remove the entry from the database (%s)\n",
			(fs::path(it->second) / name).c_str()
		);
	}
}

void Watcher::onDirectoryCreated(int wd, const std::string &name)
{
	#if not defined(NDEBUG)
		std::printf("[Watcher] onDirectoryCreated(): Directory %s created (wd: %d)\n", name.c_str(), wd);
	#endif

	auto it = folders.find(wd);
	if (it == folders.end()) {
		std::fprintf(stderr, "[Watcher] onDirectoryCreated(): Failed to find the full path for %s (wd: %d)\n",
			name.c_str(), wd
		);
		return;
	}

	auto itParent = parents.find(wd);
	if (itParent == parents.end()) {
		std::fprintf(stderr, "[Watcher] onDirectoryCreated(): Failed to find the top parent for %s (wd: %d)\n",
			name.c_str(), wd
		);
		return;
	}

	#if not defined(NDEBUG)
		std::printf("[Watcher] onDirectoryCreated(): Directory %s created in %s (parent: %s, wd: %d)\n",
			name.c_str(), it->second.c_str(), itParent->second.c_str(), wd
		);
	#endif

	auto path = fs::path(it->second) / name;
	if (!watchInternal(itParent->second, path)) {
		std::fprintf(stderr, "[Watcher] onDirectoryCreated(): Failed to watch %s in %s (parent %s)\n",
			name.c_str(), it->second.c_str(), itParent->second.c_str()
		);
	}

	// NOTE: This could be a directory that was moved from elsewhere and not just created so we need to populate the database.
	try {
		std::vector<Database::Entry> entries = {};
		for (auto &&entry: fs::recursive_directory_iterator{path, fs::directory_options::skip_permission_denied}) {
			if (entry.is_directory()) {
				continue;
			}

			std::error_code ec{};

			auto fsPath = fs::path(it->second) / name;
			auto size = fs::file_size(fsPath, ec);
			auto perms = fs::status(fsPath, ec).permissions();

			entries.emplace_back(name, it->second, itParent->second, size, perms);
		}

		if (!database->addEntries(entries)) {
			std::fprintf(stderr, "[Watcher] onDirectoryCreated(): Failed to add database entries\n");
		}
	} catch (const std::filesystem::filesystem_error &e) {
		std::fprintf(stderr, "[Watcher] onDirectoryCreated(): Filesystem error: %s\n", e.what());
	}
}

void Watcher::onDirectoryDeleted(int wd, const std::string &name)
{
	#if not defined(NDEBUG)
		std::printf("[Watcher] onDirectoryDeleted(): Directory %s deleted (wd: %d)\n", name.c_str(), wd);
	#endif

	auto it = folders.find(wd);
	if (it == folders.end()) {
		std::fprintf(stderr, "[Watcher] onDirectoryDeleted(): Failed to find the full path for %s (wd: %d)\n",
			name.c_str(), wd
		);
		return;
	}

	auto itParent = parents.find(wd);
	if (itParent == parents.end()) {
		std::fprintf(stderr, "[Watcher] onDirectoryDeleted(): Failed to find the top parent for %s (wd: %d)\n",
			name.c_str(), wd
		);
		return;
	}

	#if not defined(NDEBUG)
		std::printf("[Watcher] onDirectoryDeleted(): Directory %s deleted in %s (parent: %s, wd: %d)\n",
			name.c_str(), it->second.c_str(), itParent->second.c_str(), wd
		);
	#endif

	auto path = fs::path(it->second) / name;

	if (childDescriptors.count(itParent->second.c_str()) > 0) {
		auto &children = childDescriptors[itParent->second.c_str()];
		auto it = std::find_if(children.begin(), children.end(), [wd] (auto &&descriptor) {
			const auto &[cwd, _] = descriptor;
			return cwd == wd;
		});

		if (it != children.end()) {
			children.erase(it);
		}
	}

	parents.erase(wd);
	folders.erase(wd);

	if (inotify_rm_watch(fd, wd) == -1) {
		std::fprintf(stderr, "[Watcher] onDirectoryDeleted(): Failed to remove inotify watch (errno: %d)\n",
			errno
		);
	}

	if (!database->removeEntriesByPath(path)) {
		std::fprintf(stderr, "[Watcher] onDirectoryDeleted(): Failed to remove database entries\n");
	}
}
