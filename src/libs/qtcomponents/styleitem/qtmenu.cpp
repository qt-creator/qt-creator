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

#include "qtmenu.h"
#include "qdebug.h"
#include <qapplication.h>

QtMenu::QtMenu(QObject *parent)
    : QObject(parent)
{
    m_menu = new QMenu(0);
}

QtMenu::~QtMenu()
{
    delete m_menu;
}

void QtMenu::setTitle(const QString &title)
{
    m_title = title;
}

QString QtMenu::title() const
{
    return m_title;
}

QDeclarativeListProperty<QtMenuItem> QtMenu::menuItems()
{
    return QDeclarativeListProperty<QtMenuItem>(this, m_menuItems);
}

void QtMenu::showPopup(qreal x, qreal y)
{
    m_menu->clear();
    foreach (QtMenuItem *item, m_menuItems) {
        QAction *action = new QAction(item->text(), m_menu);
        connect(action, SIGNAL(triggered()), item, SIGNAL(selected()));
        m_menu->insertAction(0, action);
    }

    // x,y are in view coordinates, QMenu expects screen coordinates
    // ### activeWindow hack
    QPoint screenPosition = QApplication::activeWindow()->mapToGlobal(QPoint(x, y));

    m_menu->popup(screenPosition);
}

