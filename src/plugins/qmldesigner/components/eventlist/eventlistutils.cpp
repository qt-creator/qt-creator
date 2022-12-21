// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "eventlistutils.h"

#include <QApplication>
#include <QKeyEvent>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>

namespace QmlDesigner {

TabWalker::TabWalker(QObject *parent)
    : QObject(parent)
{}

bool TabWalker::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) {
            keyEvent->accept();
            int mapped = keyEvent->key() == Qt::Key_Tab ? Qt::Key_Down : Qt::Key_Up;
            int modifiers = keyEvent->nativeModifiers() & (~Qt::ShiftModifier);
            QApplication::postEvent(obj,
                                    new QKeyEvent(QEvent::KeyPress,
                                                  mapped,
                                                  static_cast<Qt::KeyboardModifier>(modifiers),
                                                  keyEvent->nativeScanCode(),
                                                  keyEvent->nativeVirtualKey(),
                                                  keyEvent->nativeModifiers(),
                                                  keyEvent->text(),
                                                  keyEvent->isAutoRepeat(),
                                                  keyEvent->count()));
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

QStandardItemModel *sourceModel(QAbstractItemModel *model)
{
    if (auto *proxy = qobject_cast<QSortFilterProxyModel *>(model))
        return sourceModel(proxy->sourceModel());

    return qobject_cast<QStandardItemModel *>(model);
}

QString uniqueName(QAbstractItemModel *model, const QString &base)
{
    if (auto *m = sourceModel(model)) {
        QList<QStandardItem *> items = m->findItems(base);
        if (items.empty())
            return base;

        int idx = 0;
        while (true) {
            QString tmp = base + QString::number(idx++);
            items = m->findItems(tmp);
            if (items.empty())
                return tmp;
        }
    }

    return QString();
}

std::string toString(AbstractView::PropertyChangeFlags flags)
{
    if (flags == 0)
        return std::string("NoAdditionalChanges");

    std::string out;
    if ((flags & AbstractView::NoAdditionalChanges) != 0)
        out += "NoAdditionalChanges ";

    if ((flags & AbstractView::PropertiesAdded) != 0) {
        if (!out.empty())
            out += "| ";

        out += "PropertiesAdded ";
    }

    if ((flags & AbstractView::EmptyPropertiesRemoved) != 0) {
        if (!out.empty())
            out += "| ";

        out += "EmptyPropertiesRemoved ";
    }
    return out;
}

void polishPalette(QTableView *view, const QColor &selectionColor)
{
    QPalette p = view->palette();
    p.setColor(QPalette::AlternateBase, p.color(QPalette::Base).lighter(120));
    p.setColor(QPalette::Highlight, selectionColor);
    view->setPalette(p);
    view->setAlternatingRowColors(true);
}

void printPropertyType(const ModelNode &node, const PropertyName &name)
{
    std::string sname = name.toStdString();

    auto prop = node.property(name);
    if (prop.isNodeProperty())
        printf("Property %s is a node-property\n", sname.c_str());

    if (prop.isVariantProperty())
        printf("Property %s is a variant-property\n", sname.c_str());

    if (prop.isNodeListProperty())
        printf("Property %s is a node-list-property\n", sname.c_str());

    if (prop.isNodeAbstractProperty())
        printf("Property %s is a node-abstract-property\n", sname.c_str());

    if (prop.isBindingProperty())
        printf("Property %s is a binding-property\n", sname.c_str());

    if (prop.isSignalHandlerProperty())
        printf("Property %s is a signal-handler-property\n", sname.c_str());
}

} // namespace QmlDesigner.
