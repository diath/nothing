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

#include "propsdialog.hpp"

#include <sys/stat.h>
#include <filesystem>

#include "core/utils.hpp"

PropsDialog::PropsDialog(QWidget *parent)
: QDialog{parent}
, name{new QLabel}
, path{new QLabel}
, parentPath{new QLabel}
, owner{new QLabel}
, group{new QLabel}
, other{new QLabel}
, modified{new QLabel}
, accessed{new QLabel}
, size{new QLabel}
, close{new QPushButton("Close")}
{
	auto layout = new QGridLayout();
	setLayout(layout);

	layout->addWidget(new QLabel("Name"), 0, 0, Qt::AlignRight);
	layout->addWidget(name, 0, 1);

	layout->addWidget(new QLabel("Path"), 1, 0, Qt::AlignRight);
	layout->addWidget(path, 1, 1);

	layout->addWidget(new QLabel("Parent"), 2, 0, Qt::AlignRight);
	layout->addWidget(parentPath, 2, 1);

	layout->addWidget(new QLabel("Perms (Owner)"), 3, 0, Qt::AlignRight);
	layout->addWidget(owner, 3, 1);

	layout->addWidget(new QLabel("Perms (Group)"), 4, 0, Qt::AlignRight);
	layout->addWidget(group, 4, 1);

	layout->addWidget(new QLabel("Perms (Other)"), 5, 0, Qt::AlignRight);
	layout->addWidget(other, 5, 1);

	layout->addWidget(new QLabel("Modified"), 6, 0, Qt::AlignRight);
	layout->addWidget(modified, 6, 1);

	layout->addWidget(new QLabel("Accessed"), 7, 0, Qt::AlignRight);
	layout->addWidget(accessed, 7, 1);

	layout->addWidget(new QLabel("Size"), 8, 0, Qt::AlignRight);
	layout->addWidget(size, 8, 1);

	layout->addWidget(close, 9, 0, 1, 2);

	connect(close, &QPushButton::clicked, [this] () {
		onClosePressed();
	});

	setWindowTitle("Properties");
}

void PropsDialog::reject()
{
	QDialog::reject();
}

void PropsDialog::load(const Database::Entry *entry)
{
	const auto &[_name, _path, _parent, _size, perms] = *entry;
	name->setText(QString::fromStdString(_name));
	path->setText(QString::fromStdString(_path));
	parentPath->setText(QString::fromStdString(_parent));

	owner->setText(QString::fromStdString(HumanReadablePermsOwner(perms)));
	group->setText(QString::fromStdString(HumanReadablePermsGroup(perms)));
	other->setText(QString::fromStdString(HumanReadablePermsOther(perms)));

	/*
		NOTE: std::filesystem does not have functions for fetching file creation and access time,
		so instead we fall back to stat() on Linux and GetFileTime() on Windows.
	*/
	#if defined(PLATFORM_LINUX)
		auto fsPath = std::filesystem::path(_path) / _name;
		struct stat s = {};
		if (stat(fsPath.c_str(), &s) == 0) {
			accessed->setText(QString::fromStdString(HumanReadableTime(s.st_atime)));
			modified->setText(QString::fromStdString(HumanReadableTime(s.st_mtime)));
		} else {
			accessed->setText("Unknown");
			modified->setText("Unknown");
		}
	#else
		// TODO: WinAPI GetFileTime()
		accessed->setText("Unknown");
		modified->setText("Unknown");
	#endif

	size->setText(QString::fromStdString(HumanReadableSize(_size)));
}

void PropsDialog::onClosePressed()
{
	hide();
}
