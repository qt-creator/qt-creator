/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#ifndef OBJECTTREE_H
#define OBJECTTREE_H

#include <QtGui/qtreewidget.h>

QT_BEGIN_NAMESPACE

class QTreeWidgetItem;

class QDeclarativeEngineDebug;
class QDeclarativeDebugObjectReference;
class QDeclarativeDebugObjectQuery;
class QDeclarativeDebugContextReference;
class QDeclarativeDebugConnection;


class ObjectTree : public QTreeWidget
{
    Q_OBJECT
public:
    ObjectTree(QDeclarativeEngineDebug *client = 0, QWidget *parent = 0);

    void setEngineDebug(QDeclarativeEngineDebug *client);

signals:
    void currentObjectChanged(const QDeclarativeDebugObjectReference &);
    void activated(const QDeclarativeDebugObjectReference &);
    void expressionWatchRequested(const QDeclarativeDebugObjectReference &, const QString &);

public slots:
    void reload(int objectDebugId);     // set the root object
    void setCurrentObject(int debugId); // select an object in the tree

protected:
    virtual void mousePressEvent(QMouseEvent *);

private slots:
    void objectFetched();
    void currentItemChanged(QTreeWidgetItem *);
    void activated(QTreeWidgetItem *);

private:
    QTreeWidgetItem *findItemByObjectId(int debugId) const;
    QTreeWidgetItem *findItem(QTreeWidgetItem *item, int debugId) const;
    void dump(const QDeclarativeDebugContextReference &, int);
    void dump(const QDeclarativeDebugObjectReference &, int);
    void buildTree(const QDeclarativeDebugObjectReference &, QTreeWidgetItem *parent);

    QDeclarativeEngineDebug *m_client;
    QDeclarativeDebugObjectQuery *m_query;
};

QT_END_NAMESPACE


#endif
