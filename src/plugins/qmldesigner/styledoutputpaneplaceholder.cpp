/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "styledoutputpaneplaceholder.h"

#include <QtCore/QChildEvent>
#include <QtCore/QFile>
#include <QtGui/QTabWidget>
#include <QtGui/QStackedWidget>
#include <QDebug>

StyledOutputpanePlaceHolder::StyledOutputpanePlaceHolder(Core::IMode *mode, QSplitter *parent) : Core::OutputPanePlaceHolder(mode, parent)
{
    QFile file(":/qmldesigner/outputpane-style.css");
    file.open(QFile::ReadOnly);
    QFile file2(":/qmldesigner/scrollbar.css");
    file2.open(QFile::ReadOnly);
    m_customStylesheet = file.readAll() + file2.readAll();
    file.close();
    file2.close();
}

void StyledOutputpanePlaceHolder::childEvent(QChildEvent *event)
{
    Core::OutputPanePlaceHolder::childEvent(event);

    if (event->type() == QEvent::ChildAdded) {
        QWidget *child = qobject_cast<QWidget*>(event->child());
        if (child) {
            QList<QTabWidget*> widgets = child->findChildren<QTabWidget*>();
            if (!widgets.isEmpty()) {
                widgets.first()->parentWidget()->ensurePolished();
                widgets.first()->parentWidget()->setStyleSheet(m_customStylesheet);
            }

        }
    } else if (event->type() == QEvent::ChildRemoved) {
        QWidget *child = qobject_cast<QWidget*>(event->child());
        if (child) {
            QList<QTabWidget*> widgets = child->findChildren<QTabWidget*>();
            if (!widgets.isEmpty())
                widgets.first()->parentWidget()->setStyleSheet(QString());
        }
    }
}
