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

#include "mainwindow.hpp"

#include <QSettings>

MainWindow::MainWindow(int argc, char **argv)
: QMainWindow{}
, pathsDialog{new PathsDialog(this)}
, input{new QLineEdit}
, table{new QTableView}
, model{new TableModel}
, timer{new QTimer{this}}
{
	qRegisterMetaType<Database::Entry>("Database::Entry");
	qRegisterMetaType<std::size_t>("std::size_t");
	qRegisterMetaType<QList<QString>>("QList");
	qRegisterMetaTypeStreamOperators<QList<QString>>("QList<QString>");

	resize(590, 530);
	setMinimumSize(590, 530);

	const auto paths = loadSettings();

	createActions();
	createStatus();

	auto centralWidget = new QWidget(this);

	auto layout = new QVBoxLayout();
	centralWidget->setLayout(layout);

	layout->addWidget(input);
	layout->addWidget(table);

	table->setModel(model);
	table->setShowGrid(false);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setSelectionMode(QAbstractItemView::SingleSelection);
	table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

	setCentralWidget(centralWidget);

	connect(input, &QLineEdit::textChanged, [this] (const QString &str) {
		onInputChanged(str.toStdString());
	});

	database = std::make_unique<Database>();
	scanner = std::make_unique<Scanner>(database.get());

	watcher = std::make_unique<Watcher>(database.get());
	watcher->run();

	if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			scanner->addPath(argv[i]);

			if (!watcher->watch(argv[i])) {
				std::printf("Failed to add %s to the watcher.\n", argv[i]);
			}
		}
	}

	for (auto &&path: paths) {
		pathsDialog->addPath(path.toStdString());
		scanner->addPath(path.toStdString());

		if (!watcher->watch(path.toStdString())) {
			std::printf("Failed to add %s to the watcher.\n", path.toStdString().c_str());
		}
	}

	scanner->run();

	statusBar()->showMessage("Scanning the paths...");

	timer->setSingleShot(true);
	connect(timer, &QTimer::timeout, [this] {
		if (queryText.empty()) {
			return;
		}

		++queryIndex;
		database->query(queryText, viewSettings.useRegexp, [this] (const std::size_t index, const auto &entry) {
			emit onEntry(index, entry);
		}, [this] () {
			emit onDone();
		});
	});

	connect(this, &MainWindow::onEntry, this, [this] (const std::size_t index, const Database::Entry &entry) {
		addEntry(index, entry);
	}, Qt::QueuedConnection);

	connect(this, &MainWindow::onDone, this, [this] () {
		fitContents();
	}, Qt::QueuedConnection);

	connect(pathsDialog, &PathsDialog::onPathAdded, [this] (const std::string &dir) {
		onPathAdded(dir);
	});
	connect(pathsDialog, &PathsDialog::onPathRemoved, [this] (const std::string &dir) {
		onPathRemoved(dir);
	});
}

MainWindow::~MainWindow()
{
	saveSettings();
}

void MainWindow::createActions()
{
	QMenu *program = menuBar()->addMenu("Program");
	program->addAction("Path Manager", [this] () {
		pathsDialog->show();
	});
	program->addAction("Quit", [this] () {
		QApplication::quit();
	});

	QMenu *view = menuBar()->addMenu("View");

	auto addToggleAction = [this, view] (const char *label, bool *flag) {
		auto action = view->addAction(label);
		action->setCheckable(true);

		if (flag) {
			action->setChecked(*flag);
		}

		connect(action, &QAction::toggled, [this, flag] (bool checked) {
			if (flag) {
				*flag = checked;
			}

			onViewSettingsChanged();
		});
	};

	addToggleAction("Regexp Search", &viewSettings.useRegexp);
	view->addSeparator();
	addToggleAction("Show Icons", &viewSettings.showIcons);
	addToggleAction("Show Size", &viewSettings.showSize);
	addToggleAction("Show Permissions", &viewSettings.showPerms);

	QMenu *help = menuBar()->addMenu("Help");
	help->addAction("Repository", [this] () {
		QDesktopServices::openUrl({"https://github.com/diath/nothing"});
	});
	help->addAction("License", [this] () {
		auto file = QFile(":/src/gui/res/license.txt");
		if (!file.open(QIODevice::ReadOnly)) {
			return;
		}

		QMessageBox::information(this, "License", QTextStream(&file).readAll());
	});
	help->addAction("About", [this] () {
		QMessageBox::information(this, "About",
			"nothing - Locate files by name instantly.\n"
			"Version 0.1\n"
			"Copyright (c) 2019 Kamil Chojnowski Y29udGFjdEBkaWF0aC5uZXQ=");
	});

	onViewSettingsChanged();
}

void MainWindow::createStatus()
{
	statusBar()->showMessage("Ready.");
}

void MainWindow::onInputChanged(const std::string &text)
{
	model->clear();
	database->stopSearchThread();
	queryText = text;

	if (text.empty()) {
		return;
	}

	timer->start(200);
}

void MainWindow::addEntry(const std::size_t index, const Database::Entry &entry)
{
	if (index != queryIndex) {
		return;
	}

	model->addEntry(entry);
}

void MainWindow::fitContents()
{
	table->resizeRowsToContents();
	table->resizeColumnToContents(0); // name
	table->resizeColumnToContents(2); // size
	table->resizeColumnToContents(3); // perms
}

void MainWindow::onPathAdded(const std::string &dir)
{
	auto result = scanner->addPath(dir);
	if (result == Scanner::AddPathResult::Ok) {
		pathsDialog->addPath(dir);
	} else {
		QString message = "Unknown Error";
		if (result == Scanner::AddPathResult::PathDoesNotExist) {
			message = "The selected path does not exist.";
		} else if (result == Scanner::AddPathResult::PathNotDirectory) {
			message = "The selected path is not a directory.";
		} else if (result == Scanner::AddPathResult::PathAlreadyAdded) {
			message = "The selected path has already been added.";
		} else if (result == Scanner::AddPathResult::ParentPathAlreadyAdded) {
			message = "The selected path has an existing parent path.";
		}

		QMessageBox::warning(this, "Path Error", message);
	}
}

void MainWindow::onPathRemoved(const std::string &dir)
{
	scanner->removePath(dir);
}

void MainWindow::onViewSettingsChanged()
{
	model->setShowIcons(viewSettings.showIcons);
	table->setColumnHidden(2, !viewSettings.showSize);
	table->setColumnHidden(3, !viewSettings.showPerms);
}

QList<QString> MainWindow::loadSettings()
{
	QSettings settings("nothing", "nothing");

	restoreGeometry(settings.value("geometry").toByteArray());
	restoreState(settings.value("windowState").toByteArray());

	viewSettings.useRegexp = settings.value("useRegexp", true).toBool();
	viewSettings.showIcons = settings.value("showIcons", true).toBool();
	viewSettings.showSize = settings.value("showSize", true).toBool();
	viewSettings.showPerms = settings.value("showPerms", true).toBool();

	return settings.value("paths").value<QList<QString>>();
}

void MainWindow::saveSettings()
{
	QSettings settings("nothing", "nothing");

	settings.setValue("geometry", saveGeometry());
	settings.setValue("windowState", saveState());

	settings.setValue("useRegexp", viewSettings.useRegexp);
	settings.setValue("showIcons", viewSettings.showIcons);
	settings.setValue("showSize", viewSettings.showSize);
	settings.setValue("showPerms", viewSettings.showPerms);

	QList<QString> paths = {};
	for (auto &&path: scanner->getPaths()) {
		paths.append(QString::fromStdString(path));
	}

	settings.setValue("paths", QVariant::fromValue(paths));
}
