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

#include <iostream>
#include <string>

#include "core/database.hpp"
#include "core/scanner.hpp"

int main(int argc, char **argv)
{
	Database db{};
	Scanner scanner{&db};
	if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			auto result = scanner.addPath(argv[i]);
			if (result == Scanner::AddPathResult::PathDoesNotExist) {
				fprintf(stderr, "Failed to add path %s, reason: path does not exist.\n", argv[i]);
			} else if (result == Scanner::AddPathResult::PathNotDirectory) {
				fprintf(stderr, "Failed to add path %s, reason: path not a directory.\n", argv[i]);
			} else if (result == Scanner::AddPathResult::PathAlreadyAdded) {
				fprintf(stderr, "Failed to add path %s, reason: path already added.\n", argv[i]);
			} else if (result == Scanner::AddPathResult::Ok) {
				printf("Added %s.\n", argv[i]);
			}
		}
	}

	scanner.run();
	std::string line{};
	while (scanner.isRunning()) {
		std::getline(std::cin, line, '\n');

		if (line == "stop") {
			scanner.stop();
		} else {
			db.queryRegexp(line, [] (const auto &e) {
				const auto &[file, _] = e;
				printf("Received %s\n", file.c_str());
			});
		}
	}

	return 0;
}
