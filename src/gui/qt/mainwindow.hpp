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

#ifndef NOTHING_MAINWINDOW_HPP
#define NOTHING_MAINWINDOW_HPP

#include <QtWidgets>

#include "core/database.hpp"
#include "core/scanner.hpp"

#if defined(PLATFORM_LINUX)
	#include "core/watcher_linux.hpp"
#elif defined(PLATFORM_WINDOWS)
	#include "core/watcher_windows.hpp"
#endif

#include "tablemodel.hpp"
#include "pathsdialog.hpp"

class MainWindow: public QMainWindow
{
	Q_OBJECT

	public:
		MainWindow(int argc, char **argv);
		~MainWindow();

	private:
		PathsDialog *pathsDialog = nullptr;

		void createActions();
		void createStatus();

		void onInputChanged(const std::string &text);

		void onPathAdded(const std::string &dir);
		void onPathRemoved(const std::string &dir);

		void onViewSettingsChanged();

		QList<QString> loadSettings();
		void saveSettings();

		QLineEdit *input = nullptr;
		QTableView *table = nullptr;
		TableModel *model = nullptr;
		QTimer *timer = nullptr;

		std::unique_ptr<Database> database = nullptr;
		std::unique_ptr<Scanner> scanner = nullptr;
		std::unique_ptr<Watcher> watcher = nullptr;

		std::size_t queryIndex = 0;
		std::string queryText = {};

		struct {
			bool useRegexp = true;
			bool showIcons = true;
			bool showSize = true;
			bool showPerms = true;
		} viewSettings;

	private slots:
		void addEntry(const std::size_t index, const Database::Entry &entry);
		void fitContents();

	signals:
		void onEntry(const std::size_t index, const Database::Entry &entry);
		void onDone();
};

#endif // NOTHING_MAINWINDOW_HPP
