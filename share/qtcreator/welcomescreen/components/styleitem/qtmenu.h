/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Components project on Qt Labs.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions contained
** in the Technology Preview License Agreement accompanying this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
****************************************************************************/

#ifndef QTMLMENU_H
#define QTMLMENU_H

#include <QtGui/qmenu.h>
#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/QDeclarativeListProperty>
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
