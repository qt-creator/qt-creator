/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef QTMLMENU_H
#define QTMLMENU_H

#include <qmenu.h>
#include <qdeclarative.h>
#include <QDeclarativeListProperty>
#include "qtmenuitem.h"
class QtMenu : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QDeclarativeListProperty<QtMenuItem> menuItems READ menuItems)
     Q_CLASSINFO("DefaultProperty", "menuItems")
public:
    QtMenu(QObject *parent = 0);
    virtual ~QtMenu();

    void setTitle(const QString &title);
    QString title() const;
    QDeclarativeListProperty<QtMenuItem> menuItems();
    Q_INVOKABLE void showPopup(qreal x, qreal y);
Q_SIGNALS:
    void selected();
private:
    QString m_title;
    QWidget *dummy;
    QMenu *m_menu;
    QList<QtMenuItem *> m_menuItems;
};

QML_DECLARE_TYPE(QtMenu)

#endif // QTMLMENU_H
