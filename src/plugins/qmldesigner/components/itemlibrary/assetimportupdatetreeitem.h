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
