/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "symbolnameitemdelegate.h"
#include "objectsmaptreeitem.h"

#include <utils/treemodel.h>

namespace Squish {
namespace Internal {

/********************************** SymbolNameItemDelegate ************************************/

SymbolNameItemDelegate::SymbolNameItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

QWidget *SymbolNameItemDelegate::createEditor(QWidget *parent,
                                              const QStyleOptionViewItem &,
                                              const QModelIndex &index) const
{
    if (auto filterModel = qobject_cast<const ObjectsMapSortFilterModel *>(index.model()))
        if (auto treeModel = qobject_cast<ObjectsMapModel *>(filterModel->sourceModel()))
            return new ValidatingContainerNameLineEdit(treeModel->allSymbolicNames(), parent);

    return new ValidatingContainerNameLineEdit(QStringList(), parent);
}

void SymbolNameItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (auto lineEdit = qobject_cast<Utils::FancyLineEdit *>(editor))
        lineEdit->setText(index.data().toString());
}

void SymbolNameItemDelegate::setModelData(QWidget *editor,
                                          QAbstractItemModel *model,
                                          const QModelIndex &index) const
{
    if (auto edit = qobject_cast<ValidatingContainerNameLineEdit *>(editor)) {
        if (!edit->isValid())
            return;
    }

    QStyledItemDelegate::setModelData(editor, model, index);
}

/******************************* ValidatingContainerNameEdit **********************************/

ValidatingContainerNameLineEdit::ValidatingContainerNameLineEdit(const QStringList &forbidden,
                                                                 QWidget *parent)
    : Utils::FancyLineEdit(parent)
    , m_forbidden(forbidden)
{
    setValidationFunction([this](FancyLineEdit *edit, QString * /*errorMessage*/) {
        if (!edit)
            return false;
        const QString &value = edit->text();
        if (value.isEmpty())
            return false;
        const QString realName = value.at(0) == ObjectsMapTreeItem::COLON
                                     ? value
                                     : ObjectsMapTreeItem::COLON + value;
        return !m_forbidden.contains(realName);
    });
}

} // namespace Internal
} // namespace Squish
