/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef OPENEDITORSVIEW_H
#define OPENEDITORSVIEW_H

#include <coreplugin/inavigationwidgetfactory.h>

#include <QAbstractProxyModel>
#include <QStyledItemDelegate>
#include <QTreeView>

namespace Core {
class IEditor;

namespace Internal {

class ProxyModel : public QAbstractProxyModel
{
    Q_OBJECT
public:
    explicit ProxyModel(QObject *parent = 0);
    QModelIndex mapFromSource(const QModelIndex & sourceIndex) const;
    QModelIndex mapToSource(const QModelIndex & proxyIndex) const;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void setSourceModel(QAbstractItemModel *sourceModel);

#if QT_VERSION >= 0x050000
    // QAbstractProxyModel::sibling is broken in Qt 5
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const;
#endif

private slots:
    void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void sourceRowsRemoved(const QModelIndex &parent, int start, int end);
    void sourceRowsInserted(const QModelIndex &parent, int start, int end);
    void sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
};

class OpenEditorsDelegate : public QStyledItemDelegate
{
public:
    explicit OpenEditorsDelegate(QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;

    mutable QModelIndex pressedIndex;
};

class OpenEditorsWidget : public QTreeView
{
    Q_OBJECT

public:
    OpenEditorsWidget();
    ~OpenEditorsWidget();

    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void handleClicked(const QModelIndex &);
    void handlePressed(const QModelIndex &);
    void updateCurrentItem(Core::IEditor*);
    void contextMenuRequested(QPoint pos);

private:
    void activateEditor(const QModelIndex &index);
    void closeEditor(const QModelIndex &index);
    using QAbstractItemView::closeEditor;

    OpenEditorsDelegate *m_delegate;
    ProxyModel *m_model;
};

class OpenEditorsViewFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT
public:
    OpenEditorsViewFactory();
    ~OpenEditorsViewFactory();
    QString displayName() const;
    int priority() const;
    Id id() const;
    QKeySequence activationSequence() const;
    Core::NavigationView createWidget();
};

} // namespace Internal
} // namespace Core


#endif // OPENEDITORSVIEW_H
