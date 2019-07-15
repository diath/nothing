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
