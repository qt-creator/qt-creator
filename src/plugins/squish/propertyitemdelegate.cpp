// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyitemdelegate.h"
#include "objectsmaptreeitem.h"

#include <utils/treemodel.h>

#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>
#include <QMouseEvent>
#include <QRegularExpression>

namespace Squish {
namespace Internal {

enum ViewColumn { Name, Operator, Value };

PropertyItemDelegate::PropertyItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

void PropertyItemDelegate::paint(QPainter *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // paint invalid values red
    if (index.column() == Value) {
        if (auto sortModel = qobject_cast<const PropertiesSortModel *>(index.model())) {
            if (auto propertiesModel = qobject_cast<PropertiesModel *>(sortModel->sourceModel())) {
                const QModelIndex idx = sortModel->mapToSource(index);
                PropertyTreeItem *item = static_cast<PropertyTreeItem *>(
                    propertiesModel->itemForIndex(idx));
                const Property &property = item->property();
                if (property.isContainer() || property.isRelativeWidget()) {
                    const ObjectsMapTreeItem *parent = propertiesModel->parentItem();
                    if (parent) {
                        if (const ObjectsMapModel *objMapModel
                            = qobject_cast<const ObjectsMapModel *>(parent->model())) {
                            if (!objMapModel->findItem(item->property().m_value))
                                opt.palette.setColor(QPalette::Text, QColor(0xff, 0, 0));
                        }
                    }
                }
            }
        }
    }
    QStyledItemDelegate::paint(painter, opt, index);
}

QWidget *PropertyItemDelegate::createEditor(QWidget *parent,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    switch (index.column()) {
    case Name: {
        auto sortModel = qobject_cast<const PropertiesSortModel *>(index.model());
        PropertiesModel *pm = qobject_cast<PropertiesModel *>(sortModel->sourceModel());

        Utils::TreeItem *self = pm->itemForIndex(sortModel->mapToSource(index));
        QStringList forbidden;
        pm->forItemsAtLevel<1>([&self, &forbidden](Utils::TreeItem *it) {
            auto item = static_cast<PropertyTreeItem *>(it);
            if (item != self)
                forbidden.append(item->property().m_name);
        });

        return new ValidatingPropertyNameLineEdit(forbidden, parent);
    }
    case Operator: {
        if (index.data().toString() == Property::OPERATOR_IS)
            return nullptr;

        QComboBox *comboBox = new QComboBox(parent);
        comboBox->addItem(Property::OPERATOR_EQUALS);
        comboBox->addItem(Property::OPERATOR_WILDCARD);
        comboBox->addItem(Property::OPERATOR_REGEX);
        comboBox->setFocusPolicy(Qt::StrongFocus);
        comboBox->setAutoFillBackground(true);
        return comboBox;
    }
    case Value: {
        auto sortModel = qobject_cast<const PropertiesSortModel *>(index.model());
        PropertiesModel *pm = qobject_cast<PropertiesModel *>(sortModel->sourceModel());

        PropertyTreeItem *self = static_cast<PropertyTreeItem *>(
            pm->itemForIndex(sortModel->mapToSource(index)));
        if (self->property().isContainer() || self->property().isRelativeWidget()) {
            auto objMapModel = qobject_cast<ObjectsMapModel *>(pm->parentItem()->model());
            return new ValidatingPropertyContainerLineEdit(objMapModel->allSymbolicNames(), parent);
        }
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
    default:
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
}

void PropertyItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (index.column() == Operator) {
        if (QComboBox *combo = qobject_cast<QComboBox *>(editor)) {
            combo->setCurrentText(index.data().toString());
            combo->showPopup();
        }
    } else if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor)) {
        lineEdit->setText(index.data().toString());
    } else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void PropertyItemDelegate::setModelData(QWidget *editor,
                                        QAbstractItemModel *model,
                                        const QModelIndex &index) const
{
    if (auto edit = qobject_cast<Utils::FancyLineEdit *>(editor)) {
        if (!edit->isValid())
            return;
    }

    QStyledItemDelegate::setModelData(editor, model, index);
}

/*********************************** ValidatingNameEdit ***************************************/

ValidatingPropertyNameLineEdit::ValidatingPropertyNameLineEdit(const QStringList &forbidden,
                                                               QWidget *parent)
    : Utils::FancyLineEdit(parent)
    , m_forbidden(forbidden)
{
    setValidationFunction([this](FancyLineEdit *edit, QString * /*errorMessage*/) {
        if (!edit)
            return false;

        const QRegularExpression identifier("^[a-zA-Z0-9_]+$");
        const QString &value = edit->text();

        return !m_forbidden.contains(value) && identifier.match(value).hasMatch();
    });
}

/*********************************** ValidatingContainerEdit **********************************/

ValidatingPropertyContainerLineEdit::ValidatingPropertyContainerLineEdit(const QStringList &allowed,
                                                                         QWidget *parent)
    : Utils::FancyLineEdit(parent)
    , m_allowed(allowed)
{
    setSpecialCompleter(new QCompleter(allowed, this));
    setValidationFunction([this](FancyLineEdit *edit, QString * /*errorMessage*/) {
        return edit && m_allowed.contains(edit->text());
    });
}

} // namespace Internal
} // namespace Squish
