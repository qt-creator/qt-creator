/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "itemlibraryitemsmodel.h"

namespace QmlDesigner {

class ItemLibraryItem;

class ItemLibraryCategory : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString categoryName READ categoryName FINAL)
    Q_PROPERTY(bool categoryVisible READ isVisible NOTIFY visibilityChanged FINAL)
    Q_PROPERTY(bool categoryExpanded READ categoryExpanded WRITE setExpanded NOTIFY expandedChanged FINAL)
    Q_PROPERTY(QObject *itemModel READ itemModel NOTIFY itemModelChanged FINAL)

public:
    ItemLibraryCategory(const QString &groupName, QObject *parent = nullptr);

    QString categoryName() const;
    bool categoryExpanded() const;
    QString sortingName() const;

    void addItem(ItemLibraryItem *item);
    QObject *itemModel();

    bool updateItemVisibility(const QString &searchText, bool *changed, bool expand = false);

    bool setVisible(bool isVisible);
    bool isVisible() const;

    void sortItems();

    void setExpanded(bool expanded);

signals:
    void itemModelChanged();
    void visibilityChanged();
    void expandedChanged();

private:
    ItemLibraryItemsModel m_itemModel;
    QString m_name;
    bool m_categoryExpanded = true;
    bool m_isVisible = true;
};

} // namespace QmlDesigner
