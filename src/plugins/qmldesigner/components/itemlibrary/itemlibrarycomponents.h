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

#ifndef ITEMLIBRARYTREEVIEW_H
#define ITEMLIBRARYTREEVIEW_H

#include <QTreeView>
#include <QStandardItemModel>
#include <QDrag>
#include <QDebug>
#include <QTimeLine>
#include <QToolButton>
#include <QStyledItemDelegate>

QT_BEGIN_NAMESPACE
class QFileSystemModel;
class QLabel;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {

class ResourceItemDelegate;

// ItemLibraryTreeView with Drag implementation
class ItemLibraryTreeView : public QTreeView {
    Q_OBJECT
public:
    explicit ItemLibraryTreeView(QWidget *parent = 0);

    virtual void startDrag(Qt::DropActions supportedActions);
    virtual void setModel(QAbstractItemModel *model);

signals:
    void itemActivated(const QString &itemName);

private slots:
    void activateItem( const QModelIndex &index);

private:
    ResourceItemDelegate *m_delegate;
};

class ResourceItemDelegate : public QStyledItemDelegate
{
public:
    explicit ResourceItemDelegate(QObject *parent=0) :
        QStyledItemDelegate(parent),m_model(0) {}

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;

    void setModel(QFileSystemModel *model);

private:
    QFileSystemModel *m_model;
};

} // namespace Internal

} // namespace QmlDesigner

#endif // ITEMLIBRARYTREEVIEW_H

