// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scattributeitemdelegate.h"
#include "mytypes.h"
#include <QComboBox>
#include <QLineEdit>
#include <QRegularExpressionValidator>

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
            QRegularExpression rx("^(?!xml)[_a-z][a-z0-9-._]*$");
            rx.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

            edit->setValidator(new QRegularExpressionValidator(rx, parent));
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
            const QStringList values = index.data(DataRole).toString().split(";");

            for (const QString &val : values)
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
