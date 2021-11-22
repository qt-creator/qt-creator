/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "checkablefilelistmodel.h"

using namespace Utils;

namespace QmlDesigner {

CheckableFileListModel::CheckableFileListModel(const FilePath &rootDir, const FilePaths &files, bool checkedByDefault, QObject *parent)
    :QStandardItemModel(parent),
      rootDir(rootDir)
{
    for (const FilePath &file: files) {
        appendRow(new CheckableStandardItem(file.toString(), checkedByDefault));
    }
}

QList<CheckableStandardItem*> CheckableFileListModel::checkedItems() const
{
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QList<QStandardItem*> allItems = findItems("*", Qt::MatchWildcard);
#else
    QList<QStandardItem*> allItems = findItems(".*", Qt::MatchRegularExpression);
#endif
    QList<CheckableStandardItem*> checkedItems;
    for (QStandardItem *standardItem : allItems) {
        CheckableStandardItem *item = static_cast<CheckableStandardItem*>(standardItem);
        if (item->isChecked())
            checkedItems.append(item);
    }

    return checkedItems;
}

QVariant CheckableFileListModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        if (role == Qt::CheckStateRole) {
            CheckableStandardItem *item = static_cast<CheckableStandardItem*>(QStandardItemModel::item(index.row()));
            return item->isChecked() ? Qt::Checked : Qt::Unchecked;
        }
        else if (role == Qt::DisplayRole) {
            QVariant data = QStandardItemModel::data(index, role);
            FilePath fullPath = FilePath::fromString(data.toString());
            FilePath relativePath = fullPath.relativeChildPath(rootDir);
            return QVariant(relativePath.toString());
        }
    }

    return QStandardItemModel::data(index, role);
}

bool CheckableFileListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::CheckStateRole)
    {
        CheckableStandardItem *item = static_cast<CheckableStandardItem*>(QStandardItemModel::item(index.row()));
        item->setChecked(value.value<bool>());

        return true;
    }

    return QStandardItemModel::setData(index, value, role);
}

CheckableStandardItem::CheckableStandardItem(const QString &text, bool checked)
    :QStandardItem(text),
      checked(checked)
{
    setFlags(flags() |= Qt::ItemIsUserCheckable);
}

void CheckableStandardItem::setChecked(bool checked)
{
    this->checked = checked;
}

bool CheckableStandardItem::isChecked() const
{
    return this->checked;
}

int CheckableStandardItem::type() const
{
    return QStandardItem::UserType + 0x74d4f1;
}

} //QmlDesigner
