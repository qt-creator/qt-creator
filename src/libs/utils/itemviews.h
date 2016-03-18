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

#pragma once

#include "utils_global.h"

static const char activationModeC[] = "ActivationMode";

#include <QListView>
#include <QListWidget>
#include <QTreeView>
#include <QTreeWidget>

#include <QKeyEvent>


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
    View(QWidget *parent = 0)
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

    void keyPressEvent(QKeyEvent *event)
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
    TreeView(QWidget *parent = 0)
        : View<QTreeView>(parent)
    {}
};

class QTCREATOR_UTILS_EXPORT TreeWidget : public View<QTreeWidget>
{
    Q_OBJECT
public:
    TreeWidget(QWidget *parent = 0)
        : View<QTreeWidget>(parent)
    {}
};

class QTCREATOR_UTILS_EXPORT ListView : public View<QListView>
{
    Q_OBJECT
public:
    ListView(QWidget *parent = 0)
        : View<QListView>(parent)
    {}
};

class QTCREATOR_UTILS_EXPORT ListWidget : public View<QListWidget>
{
    Q_OBJECT
public:
    ListWidget(QWidget *parent = 0)
        : View<QListWidget>(parent)
    {}
};

} // Utils
