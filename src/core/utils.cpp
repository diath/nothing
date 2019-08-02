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

#include <sstream>
#include <vector>

#include "utils.hpp"

std::string HumanReadablePerms(const std::filesystem::perms perms)
{
	namespace fs = std::filesystem;

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

std::string HumanReadableSize(const std::uintmax_t size)
{
	constexpr double Div = 1000.0;
	static const std::vector<std::string> Units = {
		"B", "kB", "MB", "GB", "TB"
	};

	std::size_t index = 0;
	double result = size;

	while (result > Div && (index + 1) < Units.size()) {
		result /= Div;
		++index;
	}

	std::stringstream s{};
	s << std::fixed << std::setprecision(2) << result << ' ' << Units[index];
	return s.str();
}
