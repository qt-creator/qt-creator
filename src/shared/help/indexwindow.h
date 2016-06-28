/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QAbstractProxyModel>
#include <QList>
#include <QUrl>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QHelpIndexModel;
class QModelIndex;
QT_END_NAMESPACE

namespace Utils {
class FancyLineEdit;
class NavigationTreeView;
}

namespace Help {
namespace Internal {

class IndexFilterModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    IndexFilterModel(QObject *parent);

    QModelIndex filter(const QString &filter, const QString &wildcard);
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;
    Qt::DropActions supportedDragActions() const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void setSourceModel(QAbstractItemModel *sm);

    // QAbstractProxyModel::sibling is broken in Qt 5
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;

private:
    void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void sourceRowsRemoved(const QModelIndex &parent, int start, int end);
    void sourceRowsInserted(const QModelIndex &parent, int start, int end);
    void sourceModelReset();

    QString m_filter;
    QString m_wildcard;
    QList<int> m_toSource;
};

class IndexWindow : public QWidget
{
    Q_OBJECT

public:
    IndexWindow();
    ~IndexWindow();

    void setOpenInNewPageActionVisible(bool visible);

signals:
    void linkActivated(const QUrl &link, bool newPage);
    void linksActivated(const QMap<QString, QUrl> &links,
        const QString &keyword, bool newPage);

private:
    void filterIndices(const QString &filter);
    void enableSearchLineEdit();
    void disableSearchLineEdit();
    bool eventFilter(QObject *obj, QEvent *e);
    void open(const QModelIndex &index, bool newPage = false);

    Utils::FancyLineEdit *m_searchLineEdit;
    Utils::NavigationTreeView *m_indexWidget;
    IndexFilterModel *m_filteredIndexModel;
    bool m_isOpenInNewPageActionVisible;
};

} // Internal
} // Help
