#ifndef NOTHING_MAINWINDOW_HPP
#define NOTHING_MAINWINDOW_HPP

#include <QtWidgets>

#include "core/database.hpp"
#include "core/scanner.hpp"

class MainWindow: public QMainWindow
{
	Q_OBJECT

	public:
		MainWindow(int argc, char **argv);

	private:
		void createActions();
		void createStatus();

		void onInputChanged(const std::string &text);

		QLineEdit *input = nullptr;
		QTableWidget *table = nullptr;

		std::unique_ptr<Database> database = nullptr;
		std::unique_ptr<Scanner> scanner = nullptr;
};

#endif
