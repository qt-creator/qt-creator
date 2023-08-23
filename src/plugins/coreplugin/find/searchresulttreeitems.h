// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "searchresultwindow.h"

#include <utils/searchresultitem.h>

namespace Core {
namespace Internal {

class SearchResultTreeItem
{
public:
    explicit SearchResultTreeItem(const Utils::SearchResultItem &item = {},
                                  SearchResultTreeItem *parent = nullptr);
    virtual ~SearchResultTreeItem();

    bool isLeaf() const;
    SearchResultTreeItem *parent() const;
    SearchResultTreeItem *childAt(int index) const;
    int insertionIndex(const QString &text, SearchResultTreeItem **existingItem) const;
    int insertionIndex(const Utils::SearchResultItem &item, SearchResultTreeItem **existingItem,
                       SearchResult::AddMode mode) const;
    void insertChild(int index, SearchResultTreeItem *child);
    void insertChild(int index, const Utils::SearchResultItem &item);
    void appendChild(const Utils::SearchResultItem &item);
    int childrenCount() const;
    int rowOfItem() const;
    void clearChildren();

    Qt::CheckState checkState() const;
    void setCheckState(Qt::CheckState checkState);

    bool isGenerated() const { return m_isGenerated; }
    void setGenerated(bool value) { m_isGenerated = value; }

    Utils::SearchResultItem item;

private:
    SearchResultTreeItem *m_parent;
    QList<SearchResultTreeItem *> m_children;
    bool m_isGenerated;
    Qt::CheckState m_checkState;
};

} // namespace Internal
} // namespace Core
