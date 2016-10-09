/****************************************************************************
**
** Copyright (C) 2016 Alexander Drozdov.
** Contact: adrozdoff@gmail.com
**
** This file is part of CMakeProjectManager2 plugin.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
****************************************************************************/

#include "configmodelitemdelegate.h"
#include "configmodel.h"

#include <QComboBox>

namespace CMakeProjectManager {

ConfigModelItemDelegate::ConfigModelItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{ }

ConfigModelItemDelegate::~ConfigModelItemDelegate()
{ }

QWidget* ConfigModelItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // ComboBox ony in column 2
    if (index.column() != 1)
        return QStyledItemDelegate::createEditor(parent, option, index);

    auto model = index.model();
    auto values = model->data(index, ConfigModel::ItemValuesRole).toStringList();
    if (values.isEmpty())
        return QStyledItemDelegate::createEditor(parent, option, index);

    // Create the combobox and populate it
    auto cb = new QComboBox(parent);
    cb->addItems(values);
    cb->setEditable(true);

    return cb;
}

void ConfigModelItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (QComboBox* cb = qobject_cast<QComboBox*>(editor)) {
       // get the index of the text in the combobox that matches the current value of the itenm
       QString currentText = index.data(Qt::EditRole).toString();
       int cbIndex = cb->findText(currentText);
       // if it is valid, adjust the combobox
       if (cbIndex >= 0)
           cb->setCurrentIndex(cbIndex);
       else
           cb->setEditText(currentText);
    } else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void ConfigModelItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (QComboBox* cb = qobject_cast<QComboBox*>(editor))
        // save the current text of the combo box as the current value of the item
        model->setData(index, cb->currentText(), Qt::EditRole);
    else
        QStyledItemDelegate::setModelData(editor, model, index);
}

} // namespace CMakeProjectManager

