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

#ifndef OPENPAGESWIDGET_H
#define OPENPAGESWIDGET_H

#include <QtGui/QStyledItemDelegate>
#include <QtGui/QTreeView>

namespace Help {
    namespace Internal {

class OpenPagesModel;

class OpenPagesDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit OpenPagesDelegate(QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
        const QModelIndex &index) const;

    mutable QModelIndex pressedIndex;
};

class OpenPagesWidget : public QTreeView
{
    Q_OBJECT

public:
    OpenPagesWidget(OpenPagesModel *model);
    ~OpenPagesWidget();

signals:
    void setCurrentPage(const QModelIndex &index);

    void closePage(const QModelIndex &index);
    void closePagesExcept(const QModelIndex &index);

private slots:
    void contextMenuRequested(QPoint pos);
    void handlePressed(const QModelIndex &index);
    void handleClicked(const QModelIndex &index);

private:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    OpenPagesDelegate *m_delegate;
};

    } // namespace Internal
} // namespace Help

#endif // OPENPAGESWIDGET_H
