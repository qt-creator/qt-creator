/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "core_global.h"
#include "iwelcomepage.h"

#include <utils/optional.h>

#include <QTableView>

namespace Utils { class FancyLineEdit; }

namespace Core {

class CORE_EXPORT SearchBox : public WelcomePageFrame
{
public:
    SearchBox(QWidget *parent);

    Utils::FancyLineEdit *m_lineEdit = nullptr;
};

class CORE_EXPORT GridView : public QTableView
{
public:
    explicit GridView(QWidget *parent);
protected:
    void leaveEvent(QEvent *) final;
};

using OptModelIndex = Utils::optional<QModelIndex>;

class CORE_EXPORT GridProxyModel : public QAbstractItemModel
{
public:
    void setSourceModel(QAbstractItemModel *newModel);
    QAbstractItemModel *sourceModel() const;
    QVariant data(const QModelIndex &index, int role) const final;
    Qt::ItemFlags flags(const QModelIndex &index) const final;
    bool hasChildren(const QModelIndex &parent) const final;
    void setColumnCount(int columnCount);
    int rowCount(const QModelIndex &parent) const final;
    int columnCount(const QModelIndex &parent) const final;
    QModelIndex index(int row, int column, const QModelIndex &) const final;
    QModelIndex parent(const QModelIndex &) const final;

    OptModelIndex mapToSource(const QModelIndex &proxyIndex) const;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;

    static constexpr int GridItemWidth = 230;
    static constexpr int GridItemHeight = 230;
    static constexpr int GridItemGap = 10;
    static constexpr int TagsSeparatorY = GridItemHeight - 60;

private:
    QAbstractItemModel *m_sourceModel = nullptr;
    int m_columnCount = 1;
};

} // namespace Core
