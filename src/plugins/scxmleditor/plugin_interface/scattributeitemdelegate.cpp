/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
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

#include "scattributeitemdelegate.h"
#include "mytypes.h"
#include <QComboBox>
#include <QLineEdit>
#include <QRegExp>
#include <QRegExpValidator>

using namespace ScxmlEditor::PluginInterface;

SCAttributeItemDelegate::SCAttributeItemDelegate(QObject *parent)
    : AttributeItemDelegate(parent)
{
}

QWidget *SCAttributeItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)

    switch (index.data(DataTypeRole).toInt()) {
    case QVariant::StringList: {
        auto combo = new QComboBox(parent);
        combo->setFocusPolicy(Qt::StrongFocus);
        return combo;
    }
    case QVariant::String: {
        if (index.column() == 0) {
            auto edit = new QLineEdit(parent);
            edit->setFocusPolicy(Qt::StrongFocus);
            QRegExp rx("^(?!xml)[_a-z][a-z0-9-._]*$");
            rx.setCaseSensitivity(Qt::CaseInsensitive);
            edit->setValidator(new QRegExpValidator(rx, parent));
            return edit;
        }
    }
    default:
        break;
    }

    return QStyledItemDelegate::createEditor(parent, option, index);
}

void SCAttributeItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    if (editor)
        editor->setGeometry(option.rect);
}

void SCAttributeItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    switch (index.data(DataTypeRole).toInt()) {
    case QVariant::StringList: {
        auto combo = qobject_cast<QComboBox*>(editor);
        if (combo) {
            combo->clear();
            QStringList values = index.data(DataRole).toString().split(";");

            foreach (QString val, values)
                combo->addItem(val);

            combo->setCurrentText(index.data().toString());
            return;
        }
        break;
    }
    default:
        break;
    }

    QStyledItemDelegate::setEditorData(editor, index);
}

void SCAttributeItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    auto combo = qobject_cast<QComboBox*>(editor);
    if (combo) {
        model->setData(index, combo->currentText());
        return;
    }

    QStyledItemDelegate::setModelData(editor, model, index);
}
