#include "mainwindow.hpp"

MainWindow::MainWindow(int argc, char **argv)
: QMainWindow{}
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

	table->setColumnCount(2);
	table->setHorizontalHeaderLabels({"File", "Folder"});

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

		scanner->run();
		statusBar()->showMessage("Scanning the paths...");
	}
}

void MainWindow::createActions()
{
	QMenu *program = menuBar()->addMenu("Program");
	QMenu *view = menuBar()->addMenu("View");
	QMenu *help = menuBar()->addMenu("Help");

	(void) program; (void) view; (void) help;
}

void MainWindow::createStatus()
{
	statusBar()->showMessage("Ready.");
}

void MainWindow::onInputChanged(const std::string &text)
{
	table->setRowCount(0);

	if (text.empty()) {
		return;
	}

	std::size_t index = 0;
	database->query(text, [this, &index] (const auto &e) {
		auto &[file, path] = e;
		table->insertRow(table->rowCount());

		auto fileItem = new QTableWidgetItem();
		fileItem->setText(QString::fromStdString(file));

		auto pathItem = new QTableWidgetItem();
		pathItem->setText(QString::fromStdString(path));

		table->setItem(table->rowCount() - 1, 0, fileItem);
		table->setItem(table->rowCount() - 1, 1, pathItem);
	});
}
