/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef IOUTPUTPANE_H
#define IOUTPUTPANE_H

#include "core_global.h"

#include <QObject>
#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT IOutputPane : public QObject
{
    Q_OBJECT

public:
    IOutputPane(QObject *parent = 0) : QObject(parent) {}

    virtual QWidget *outputWidget(QWidget *parent) = 0;
    virtual QList<QWidget *> toolBarWidgets() const = 0;
    virtual QString displayName() const = 0;

    // -1 don't show in statusBar
    // 100...0 show at front...end
    virtual int priorityInStatusBar() const = 0;

    virtual void clearContents() = 0;
    virtual void visibilityChanged(bool visible) = 0;

    // This function is called to give the outputwindow focus
    virtual void setFocus() = 0;
    // Whether the outputpane has focus
    virtual bool hasFocus() const = 0;
    // Whether the outputpane can be focused at the moment.
    // (E.g. the search result window does not want to be focused if the are no results.)
    virtual bool canFocus() const = 0;

    virtual bool canNavigate() const = 0;
    virtual bool canNext() const = 0;
    virtual bool canPrevious() const = 0;
    virtual void goToNext() = 0;
    virtual void goToPrev() = 0;

    enum Flag { NoModeSwitch = 0, ModeSwitch = 1, WithFocus = 2, EnsureSizeHint = 4};
    Q_DECLARE_FLAGS(Flags, Flag)

public slots:
    void popup(int flags) { emit showPage(flags); }

    void hide() { emit hidePage(); }
    void toggle(int flags) { emit togglePage(flags); }
    void navigateStateChanged() { emit navigateStateUpdate(); }
    void flash() { emit flashButton(); }
    void setIconBadgeNumber(int number) { emit setBadgeNumber(number); }

signals:
    void showPage(int flags);
    void hidePage();
    void togglePage(int flags);
    void navigateStateUpdate();
    void flashButton();
    void setBadgeNumber(int number);
};

} // namespace Core

 Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IOutputPane::Flags)

#endif // IOUTPUTPANE_H
