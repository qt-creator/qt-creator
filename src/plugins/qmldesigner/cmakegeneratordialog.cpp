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


#include "cmakegeneratordialog.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QLayout>
#include <QLabel>
#include <QListView>

using namespace Utils;

namespace QmlDesigner {
namespace GenerateCmake {

CmakeGeneratorDialog::CmakeGeneratorDialog(const FilePath &rootDir, const FilePaths &files)
    : QDialog()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

    auto *okButton = buttons->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    model = new CheckableFileListModel(rootDir, files, this);

    QListView *list = new QListView(this);
    list->setModel(model);

    layout->addWidget(list);
    layout->addWidget(buttons);
}

FilePaths CmakeGeneratorDialog::getFilePaths()
{
    FilePaths paths;

    QList<CheckableStandardItem*> items = model->checkedItems();
    for (CheckableStandardItem *item: items) {
        paths.append(FilePath::fromString(item->text()));
    }

    return paths;
}

CheckableFileListModel::CheckableFileListModel(const FilePath &rootDir, const FilePaths &files, QObject *parent)
    :QStandardItemModel(parent),
      rootDir(rootDir)
{
    for (const FilePath &file: files) {
        appendRow(new CheckableStandardItem(file.toString(), true));
    }
}

QList<CheckableStandardItem*> CheckableFileListModel::checkedItems() const
{
    QList<QStandardItem*> allItems = findItems(".*", Qt::MatchRegularExpression);
    QList<CheckableStandardItem*> checkedItems;
    for (QStandardItem *standardItem : allItems) {
        CheckableStandardItem *item = static_cast<CheckableStandardItem*>(standardItem);
        if (item->isChecked())
            checkedItems.append(item);
    }

    return checkedItems;
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

QVariant CheckableFileListModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        if (role == Qt::CheckStateRole) {
            CheckableStandardItem *item = static_cast<CheckableStandardItem*>(QStandardItemModel::item(index.row()));
            return item->isChecked() ? Qt::Checked : Qt::Unchecked;
        }
        else if (role == Qt::DisplayRole) {
            QVariant data = QStandardItemModel::data(index, role);
            QString relativePath = data.toString().remove(rootDir.toString());
            return QVariant(relativePath);
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

}
}
