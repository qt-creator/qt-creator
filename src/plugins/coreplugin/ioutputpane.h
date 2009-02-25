/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef IOUTPUTPANE_H
#define IOUTPUTPANE_H

#include "core_global.h"

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtGui/QWidget>

namespace Core {

class CORE_EXPORT IOutputPane : public QObject
{
    Q_OBJECT
public:
    IOutputPane(QObject *parent = 0) : QObject(parent) {}
    virtual ~IOutputPane() {}

    virtual QWidget *outputWidget(QWidget *parent) = 0;
    virtual QList<QWidget*> toolBarWidgets(void) const = 0;
    virtual QString name() const = 0;

    // -1 don't show in statusBar
    // 100...0 show at front...end
    virtual int priorityInStatusBar() const = 0;

    virtual void clearContents() = 0;
    virtual void visibilityChanged(bool visible) = 0;

    // This function is called to give the outputwindow focus
    virtual void setFocus() = 0;
    // Wheter the outputpane has focus
    virtual bool hasFocus() = 0;
    // Wheter the outputpane can be focused at the moment.
    // (E.g. the search result window doesn't want to be focussed if the are no results.)
    virtual bool canFocus() = 0;
public slots:
    void popup()
    {
        popup(true);
    }
    void popup(bool withFocus)
    {
        emit showPage(withFocus);
    }

    void hide()
    {
        emit hidePage();
    }

    void toggle()
    {
        toggle(true);
    }

    void toggle(bool withFocusIfShown)
    {
        emit togglePage(withFocusIfShown);
    }

signals:
    void showPage(bool withFocus);
    void hidePage();
    void togglePage(bool withFocusIfShown);
};

} // namespace Core

#endif // IOUTPUTPANE_H
