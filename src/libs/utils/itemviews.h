// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QKeyEvent>
#include <QListView>
#include <QListWidget>
#include <QTreeView>
#include <QTreeWidget>

static const char activationModeC[] = "ActivationMode";

namespace Utils {

enum ActivationMode {
    DoubleClickActivation = 0,
    SingleClickActivation = 1,
    PlatformDefaultActivation = 2
};

template<class BaseT>
class View : public BaseT
{
public:
    View(QWidget *parent = nullptr)
        : BaseT(parent)
    {}
    void setActivationMode(ActivationMode mode)
    {
        if (mode == PlatformDefaultActivation)
            BaseT::setProperty(activationModeC, QVariant());
        else
            BaseT::setProperty(activationModeC, QVariant(bool(mode)));
    }

    ActivationMode activationMode() const
    {
        QVariant v = BaseT::property(activationModeC);
        if (!v.isValid())
            return PlatformDefaultActivation;
        return v.toBool() ? SingleClickActivation : DoubleClickActivation;
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        // Note: This always eats the event
        // whereas QAbstractItemView never eats it
        if ((event->key() == Qt::Key_Return
                || event->key() == Qt::Key_Enter)
                && event->modifiers() == 0
                && BaseT::currentIndex().isValid()
                && BaseT::state() != QAbstractItemView::EditingState) {
            emit BaseT::activated(BaseT::currentIndex());
            return;
        }
        BaseT::keyPressEvent(event);
    }

};

class QTCREATOR_UTILS_EXPORT TreeView : public View<QTreeView>
{
    Q_OBJECT
public:
    TreeView(QWidget *parent = nullptr)
        : View<QTreeView>(parent)
    {}
};

class QTCREATOR_UTILS_EXPORT TreeWidget : public View<QTreeWidget>
{
    Q_OBJECT
public:
    TreeWidget(QWidget *parent = nullptr)
        : View<QTreeWidget>(parent)
    {}
};

class QTCREATOR_UTILS_EXPORT ListView : public View<QListView>
{
    Q_OBJECT
public:
    ListView(QWidget *parent = nullptr)
        : View<QListView>(parent)
    {}
};

class QTCREATOR_UTILS_EXPORT ListWidget : public View<QListWidget>
{
    Q_OBJECT
public:
    ListWidget(QWidget *parent = nullptr)
        : View<QListWidget>(parent)
    {}
};

} // Utils
