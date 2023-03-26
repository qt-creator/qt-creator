// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
