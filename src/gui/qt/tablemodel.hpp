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

#ifndef NOTHING_TABLEMODEL_HPP
#define NOTHING_TABLEMODEL_HPP

#include <QAbstractTableModel>
#include <QPixmap>

#include "core/database.hpp"
#include "core/utils.hpp"

#include <unordered_map>
#include <vector>

class TableModel: public QAbstractTableModel
{
	Q_OBJECT

	public:
		TableModel(QObject *parent = nullptr);

		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex &parent = QModelIndex()) const;

		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

		void addEntry(const Database::Entry &entry);
		void clear();

		void setShowIcons(const bool show);

	private:
		std::vector<Database::Entry> entries = {};
		std::unordered_map<FileType, QPixmap> icons = {};
		bool showIcons = true;
};

#endif
