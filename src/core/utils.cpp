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
#include <sstream>
#include <tuple>
#include <vector>

#include "utils.hpp"

namespace fs = std::filesystem;

std::string HumanReadablePerms(const fs::perms perms)
{
	std::stringstream s{};
	s << ((perms & fs::perms::owner_read)   != fs::perms::none ? "r" : "-")
	  << ((perms & fs::perms::owner_write)  != fs::perms::none ? "w" : "-")
	  << ((perms & fs::perms::owner_exec)   != fs::perms::none ? "x" : "-")
	  << ((perms & fs::perms::group_read)   != fs::perms::none ? "r" : "-")
	  << ((perms & fs::perms::group_write)  != fs::perms::none ? "w" : "-")
	  << ((perms & fs::perms::group_exec)   != fs::perms::none ? "x" : "-")
	  << ((perms & fs::perms::others_read)  != fs::perms::none ? "r" : "-")
	  << ((perms & fs::perms::others_write) != fs::perms::none ? "w" : "-")
	  << ((perms & fs::perms::others_exec)  != fs::perms::none ? "x" : "-");

	return s.str();
}

std::string HumanReadablePermsOwner(const fs::perms perms)
{
	std::stringstream s{};
	bool added = false;

	auto append = [perms, &s, &added] (const fs::perms flag, const std::string &label) {
		if ((perms & flag) != fs::perms::none) {
			if (added) {
				s << ", ";
			}

			s << label;
			added = true;
		}
	};

	append(fs::perms::owner_read, "Read");
	append(fs::perms::owner_write, "Write");
	append(fs::perms::owner_exec, "Execute");

	return s.str();
}

std::string HumanReadablePermsGroup(const fs::perms perms)
{
	std::stringstream s{};
	bool added = false;

	auto append = [perms, &s, &added] (const fs::perms flag, const std::string &label) {
		if ((perms & flag) != fs::perms::none) {
			if (added) {
				s << ", ";
			}

			s << label;
			added = true;
		}
	};

	append(fs::perms::group_read, "Read");
	append(fs::perms::group_write, "Write");
	append(fs::perms::group_exec, "Execute");

	return s.str();
}

std::string HumanReadablePermsOther(const fs::perms perms)
{
	std::stringstream s{};
	bool added = false;

	auto append = [perms, &s, &added] (const fs::perms flag, const std::string &label) {
		if ((perms & flag) != fs::perms::none) {
			if (added) {
				s << ", ";
			}

			s << label;
			added = true;
		}
	};

	append(fs::perms::others_read, "Read");
	append(fs::perms::others_write, "Write");
	append(fs::perms::others_exec, "Execute");

	return s.str();
}

std::string HumanReadableSize(const std::uintmax_t size)
{
	constexpr double Div = 1000.0;
	static const std::vector<std::string> Units = {
		"bytes", "kB", "MB", "GB", "TB"
	};

	std::size_t index = 0;
	double result = size;

	while (result > Div && (index + 1) < Units.size()) {
		result /= Div;
		++index;
	}

	std::stringstream s{};
	if (index == 0) {
		s << std::fixed << std::setprecision(0) << result << ' ' << Units[index];
	} else {
		s << std::fixed << std::setprecision(2) << result << ' ' << Units[index];
	}

	return s.str();
}

std::string HumanReadableTime(const std::time_t time)
{
	auto tm = std::localtime(&time);

	char buffer[512] = {0};
	std::strftime(buffer, sizeof(buffer), "%d %b %Y - %H:%M:%S", tm);
	return buffer;
}

FileType GetFileType(std::string name)
{
	static const std::tuple<std::string, FileType> FileTypes[] = {
		// Document
		{".txt", FileType::Document},
		{".doc", FileType::Document},
		{".pdf", FileType::Document},
		{".tex", FileType::Document},
		{".rtf", FileType::Document},
		// Image
		{".jpg", FileType::Image},
		{".jpeg", FileType::Image},
		{".png", FileType::Image},
		{".bmp", FileType::Image},
		{".gif", FileType::Image},
		{".tiff", FileType::Image},
		{".ppm", FileType::Image},
		// Video
		{".webm", FileType::Video},
		{".mkv", FileType::Video},
		{".flv", FileType::Video},
		{".avi", FileType::Video},
		{".mp4", FileType::Video},
		{".flv", FileType::Video},
		// Audio
		{".3gp", FileType::Audio},
		{".aac", FileType::Audio},
		{".m4a", FileType::Audio},
		{".mp3", FileType::Audio},
		{".ogg", FileType::Audio},
		{".opus", FileType::Audio},
		{".wav", FileType::Audio},
		// Archive
		{".zip", FileType::Archive},
		{".rar", FileType::Archive},
		{".tar", FileType::Archive},
		{".gz", FileType::Archive},
		{".xz", FileType::Archive},
		{".bz2", FileType::Archive},
		{".7z", FileType::Archive},
		{".Z", FileType::Archive},
		{".tgz", FileType::Archive},
		{".tbz2", FileType::Archive},
		// System
		{".exe", FileType::System},
		{".dll", FileType::System},
		{".sys", FileType::System},
		{".so", FileType::System},
	};

	const auto size = name.size();
	if (!size) {
		return FileType::Generic;
	}

	std::transform(name.begin(), name.end(), name.begin(), [] (const unsigned char ch) {
		return std::tolower(ch);
	});

	for (const auto &pair: FileTypes) {
		const auto &[ext, type] = pair;
		if (ext.size() > size) {
			continue;
		}

		auto index = name.rfind(ext);
		if (index == std::string::npos) {
			continue;
		}

		if (name.substr(index) == ext) {
			return type;
		}
	}

	return FileType::Generic;
}
