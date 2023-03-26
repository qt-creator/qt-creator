// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QList>
#include <QFileInfo>

namespace QmlDesigner {
namespace Internal {

class AssetImportUpdateTreeItem
{
public:
    explicit AssetImportUpdateTreeItem(const QFileInfo &info,
                                       AssetImportUpdateTreeItem *parent = nullptr);
    virtual ~AssetImportUpdateTreeItem();

    AssetImportUpdateTreeItem *parent() const;
    AssetImportUpdateTreeItem *childAt(int index) const;
    int childCount() const;
    int rowOfItem() const;
    void clear();

    Qt::CheckState checkState() const { return m_checkState; }
    void setCheckState(Qt::CheckState checkState) { m_checkState = checkState; }
    const QFileInfo &fileInfo() const { return m_fileInfo; }
    void setFileInfo(const QFileInfo &info) { m_fileInfo = info; }
    void removeChild(AssetImportUpdateTreeItem *item);
    const QList<AssetImportUpdateTreeItem *> &children() const { return m_children; }

private:
    void appendChild(AssetImportUpdateTreeItem *item);

    AssetImportUpdateTreeItem *m_parent;
    QList<AssetImportUpdateTreeItem *> m_children;
    Qt::CheckState m_checkState = Qt::Unchecked;
    QFileInfo m_fileInfo;
};

} // namespace Internal
} // namespace QmlDesigner
