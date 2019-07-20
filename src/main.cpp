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
			db.query(line, [] (const auto &e) {
				const auto &[file, _] = e;
				printf("Received %s\n", file.c_str());
			});
		}
	}

	return 0;
}
