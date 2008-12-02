/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
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
