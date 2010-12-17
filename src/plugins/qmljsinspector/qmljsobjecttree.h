/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef OBJECTTREE_H
#define OBJECTTREE_H

#include <qmljsprivateapi.h>
#include <QtGui/QTreeWidget>

QT_BEGIN_NAMESPACE

class QTreeWidgetItem;

QT_END_NAMESPACE

namespace QmlJSInspector {
namespace Internal {

class QmlJSObjectTree : public QTreeWidget
{
    Q_OBJECT
public:
    QmlJSObjectTree(QWidget *parent = 0);

signals:
    void currentObjectChanged(const QDeclarativeDebugObjectReference &);
    void activated(const QDeclarativeDebugObjectReference &);
    void expressionWatchRequested(const QDeclarativeDebugObjectReference &, const QString &);
    void contextHelpIdChanged(const QString &contextHelpId);

public slots:
    void setCurrentObject(int debugId); // select an object in the tree

protected:
    virtual void contextMenuEvent(QContextMenuEvent *);

private slots:
    void currentItemChanged(QTreeWidgetItem *);
    void activated(QTreeWidgetItem *);
    void selectionChanged();
    void goToFile();

private:
    QTreeWidgetItem *findItemByObjectId(int debugId) const;
    QTreeWidgetItem *findItem(QTreeWidgetItem *item, int debugId) const;
    void buildTree(const QDeclarativeDebugObjectReference &, QTreeWidgetItem *parent);

    QTreeWidgetItem *m_clickedItem;
    QAction *m_addWatchAction;
    QAction *m_goToFileAction;
    int m_currentObjectDebugId;
};

} // Internal
} // QmlJSInspector

#endif
