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

#include "pathsdialog.hpp"

PathsDialog::PathsDialog(QWidget *parent)
: QDialog{parent}
, list{new QListWidget}
, path{new QLineEdit}
, browse{new QPushButton("Browse")}
, add{new QPushButton("Add")}
, remove{new QPushButton("Remove")}
, close{new QPushButton("Close")}
{
	auto layout = new QVBoxLayout();
	setLayout(layout);

	auto pathHolder = new QWidget();
	auto pathLayout = new QHBoxLayout();
	pathLayout->setMargin(0);
	pathHolder->setLayout(pathLayout);
	pathLayout->addWidget(path);
	pathLayout->addWidget(browse);

	layout->addWidget(list);
	layout->addWidget(pathHolder);
	layout->addWidget(add);
	layout->addWidget(remove);
	layout->addWidget(close);

	path->setReadOnly(true);

	connect(browse, &QPushButton::pressed, [this] () {
		onBrowsePressed();
	});
	connect(add, &QPushButton::pressed, [this] () {
		onAddPressed();
	});
	connect(remove, &QPushButton::pressed, [this] () {
		onRemovePressed();
	});
	connect(close, &QPushButton::pressed, [this] () {
		onClosePressed();
	});

	setWindowTitle("Path Manager");
}

void PathsDialog::onBrowsePressed()
{
	auto dir = QFileDialog::getExistingDirectory(this, "Select Directory");
	if (dir.isEmpty()) {
		return;
	}

	path->setText(dir);
}

void PathsDialog::onAddPressed()
{
	auto dir = path->text();
	if (dir.isEmpty()) {
		return;
	}

	// NOTE: We do not add the path to the ListWidget here, instead we wait for the MainWindow to add the path,
	// after it's done verifying that it's been successfully added.
	emit onPathAdded(dir.toStdString());
}

void PathsDialog::onRemovePressed()
{
	auto item = list->currentItem();
	if (!item) {
		return;
	}

	auto dir = item->text();
	if (dir.isEmpty()) {
		return;
	}

	list->removeItemWidget(item);

	// NOTE: The Qt docs say: "Items removed from a list widget will not be managed by Qt, and will need to be deleted manually.".
	delete item;

	emit onPathRemoved(dir.toStdString());
}

void PathsDialog::onClosePressed()
{
	hide();
}

void PathsDialog::addPath(const std::string &dir)
{
	list->addItem(QString::fromStdString(dir));
}
