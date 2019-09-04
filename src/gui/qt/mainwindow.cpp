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

#include "core/utils.hpp"

namespace {

std::string FileTypeToResource(const FileType type)
{
	switch (type) {
		case FileType::Archive:  return ":src/gui/res/archive.svg";  break;
		case FileType::Audio:    return ":src/gui/res/audio.svg";    break;
		case FileType::Document: return ":src/gui/res/document.svg"; break;
		case FileType::Image:    return ":src/gui/res/image.svg";    break;
		case FileType::System:   return ":src/gui/res/system.svg";   break;
		case FileType::Video:    return ":src/gui/res/video.svg";    break;
		case FileType::Generic:  return {};                           break;
	}

	return {};
}


} // namespace

MainWindow::MainWindow(int argc, char **argv)
: QMainWindow{}
, pathsDialog{new PathsDialog(this)}
, input{new QLineEdit}
, table{new QTableWidget}
{
	resize(590, 530);
	setMinimumSize(590, 530);

	createActions();
	createStatus();

	auto centralWidget = new QWidget(this);

	auto layout = new QVBoxLayout();
	centralWidget->setLayout(layout);

	layout->addWidget(input);
	layout->addWidget(table);

	table->setColumnCount(4);
	table->setHorizontalHeaderLabels({"File", "Folder", "Size", "Perms"});
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setShowGrid(false);
	table->verticalHeader()->setVisible(false);
	table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

	setCentralWidget(centralWidget);

	connect(input, &QLineEdit::textChanged, [this] (const QString &str) {
		onInputChanged(str.toStdString());
	});

	database = std::make_unique<Database>();
	scanner = std::make_unique<Scanner>(database.get());

	if (argc > 1) {
		for (int i = 0; i < argc; ++i) {
			scanner->addPath(argv[i]);
		}
	}

	scanner->run();
	statusBar()->showMessage("Scanning the paths...");

	qRegisterMetaType<Database::Entry>("Database::Entry");
	qRegisterMetaType<std::size_t>("std::size_t");

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
	(void) view;

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
}

void MainWindow::createStatus()
{
	statusBar()->showMessage("Ready.");
}

void MainWindow::onInputChanged(const std::string &text)
{
	table->setRowCount(0);
	database->stopSearchThread();

	if (text.empty()) {
		return;
	}

	++queryIndex;
	database->query(text, false, [this] (const std::size_t index, const auto &entry) {
		emit onEntry(index, entry);
	}, [this] () {
		emit onDone();
	});
}

void MainWindow::addEntry(const std::size_t index, const Database::Entry &entry)
{
	if (index != queryIndex) {
		return;
	}

	const auto &[file, path, _, size, perms] = entry;
	table->insertRow(table->rowCount());

	auto createItem = [] () {
		auto item = new QTableWidgetItem();
		item->setFlags(item->flags() &  ~Qt::ItemIsEditable);
		return item;
	};

	auto fileItem = createItem();
	fileItem->setText(QString::fromStdString(file));

	auto pathItem = createItem();
	pathItem->setText(QString::fromStdString(path));

	auto sizeItem = createItem();
	sizeItem->setText(QString("%1").arg(QString::fromStdString(HumanReadableSize(size))));
	sizeItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);

	auto permsItem = createItem();
	permsItem->setText(QString("%1").arg(QString::fromStdString(HumanReadablePerms(perms))));
	permsItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

	table->setItem(table->rowCount() - 1, 0, fileItem);
	table->setItem(table->rowCount() - 1, 1, pathItem);
	table->setItem(table->rowCount() - 1, 2, sizeItem);
	table->setItem(table->rowCount() - 1, 3, permsItem);

	auto res = FileTypeToResource(GetFileType(file));
	if (!res.empty()) {
		fileItem->setIcon(QIcon(QString::fromStdString(res)));
	}
}

void MainWindow::fitContents()
{
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
		}

		QMessageBox::warning(this, "Path Error", message);
	}
}

void MainWindow::onPathRemoved(const std::string &dir)
{
	scanner->removePath(dir);
}
