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

#include "tablemodel.hpp"

namespace {


constexpr int IconWidth = 32;
constexpr int IconHeight = 32;

std::string FileTypeToResource(const FileType type)
{
	switch (type) {
		case FileType::Archive:  return ":src/gui/res/archive.svg";  break;
		case FileType::Audio:    return ":src/gui/res/audio.svg";    break;
		case FileType::Document: return ":src/gui/res/document.svg"; break;
		case FileType::Image:    return ":src/gui/res/image.svg";    break;
		case FileType::System:   return ":src/gui/res/system.svg";   break;
		case FileType::Video:    return ":src/gui/res/video.svg";    break;
		case FileType::Generic:  return {};                          break;
	}

	return {};
}


} // namespace

TableModel::TableModel(QObject *parent/* = nullptr*/)
: QAbstractTableModel(parent)
{
	for (const auto &type: {
		FileType::Archive, FileType::Audio, FileType::Document,
		FileType::Image, FileType::System, FileType::Video,
		FileType::Generic,
	}) {
		const auto res = FileTypeToResource(type);
		if (res.empty()) {
			continue;
		}

		icons[type] = QPixmap(QString::fromStdString(res)).scaled(IconWidth, IconHeight);
	}
}

int TableModel::rowCount(const QModelIndex &parent/* = QModelIndex()*/) const
{
	return entries.size();
}

int TableModel::columnCount(const QModelIndex &parent/* = QModelIndex()*/) const
{
	return 4;
}

QVariant TableModel::data(const QModelIndex &index, int role/* = Qt::DisplayRole*/) const
{
	int position = index.row();
	if (position < 0 || position >= static_cast<int>(entries.size())) {
		return {};
	}

	const auto &[file, path, _, size, perms] = entries[position];
	if (role == Qt::DisplayRole) {
		switch (index.column()) {
			case 0: return QString::fromStdString(file);
			case 1: return QString::fromStdString(path);
			case 2: return QString::fromStdString(HumanReadableSize(size));
			case 3: return QString::fromStdString(HumanReadablePerms(perms));
		}
	} else if (showIcons && role == Qt::DecorationRole) {
		switch (index.column()) {
			case 0: {
				const auto it = icons.find(GetFileType(file));
				if (it == icons.end()) {
					return {};
				}

				return it->second;
			} break;
		}
	}

	return {};
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role/* = Qt::DisplayRole*/) const
{
	if (role != Qt::DisplayRole) {
		return {};
	}

	if (orientation != Qt::Horizontal) {
		return {};
	}

	switch (section) {
		case 0: return QString{"File"};   break;
		case 1: return QString{"Folder"}; break;
		case 2: return QString{"Size"};   break;
		case 3: return QString{"Perms"};  break;
	}

	return {};
}

void TableModel::addEntry(const Database::Entry &entry)
{
	entries.emplace_back(entry);
	emit layoutChanged();
}

void TableModel::clear()
{
	entries.clear();
	emit layoutChanged();
}

void TableModel::setShowIcons(const bool show)
{
	showIcons = show;
	emit layoutChanged();
}

Database::Entry *TableModel::entry(const int index)
{
	if (index < 0 || index >= static_cast<int>(entries.size())) {
		return nullptr;
	}

	return &entries[index];
}
