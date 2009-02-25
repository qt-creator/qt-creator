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

#ifndef INAVIGATIONWIDGET_H
#define INAVIGATIONWIDGET_H

#include <coreplugin/core_global.h>
#include <QtCore/QObject>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QToolButton;
class QKeySequence;
class QWidget;
QT_END_NAMESPACE

namespace Core {

struct NavigationView
{
    QWidget *widget;
    QList<QToolButton *> doockToolBarWidgets;
};

class CORE_EXPORT INavigationWidgetFactory : public QObject
{
    Q_OBJECT
public:
    INavigationWidgetFactory();
    virtual ~INavigationWidgetFactory();

    virtual QString displayName() = 0;
    virtual QKeySequence activationSequence();
    // This design is not optimal, think about it again once we need to extend it
    // It could be implemented as returning an object which has both the widget 
    // and the docktoolbar widgets
    // Similar to how IView
    virtual NavigationView createWidget() = 0;

    // Read and store settings for the widget, created by this factory
    // and beeing at position position. (The position is important since
    // a certain type of widget could exist multiple times.)
    virtual void saveSettings(int position, QWidget *widget);
    virtual void restoreSettings(int position, QWidget *widget);
};

} // namespace Core

#endif // INAVIGATIONWIDGET_H
