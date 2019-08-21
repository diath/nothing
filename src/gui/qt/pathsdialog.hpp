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

#ifndef NOTHING_PATHSDIALOG_HPP
#define NOTHING_PATHSDIALOG_HPP

#include <QtWidgets>

class PathsDialog: public QDialog
{
	Q_OBJECT

	public:
		PathsDialog(QWidget *parent);

		void addPath(const std::string &dir);

	private:
		QListWidget *list = nullptr;
		QLineEdit *path = nullptr;
		QPushButton *browse = nullptr;
		QPushButton *add = nullptr;
		QPushButton *remove = nullptr;
		QPushButton *close = nullptr;

		void onBrowsePressed();
		void onAddPressed();
		void onRemovePressed();
		void onClosePressed();

	signals:
		void onPathAdded(const std::string &path);
		void onPathRemoved(const std::string &path);
};


#endif // NOTHING_PATHSDIALOG_HPP
