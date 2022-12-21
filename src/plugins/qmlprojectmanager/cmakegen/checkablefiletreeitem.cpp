// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "checkablefiletreeitem.h"

using namespace Utils;

namespace QmlProjectManager {

CheckableFileTreeItem::CheckableFileTreeItem(const FilePath &filePath)
    :QStandardItem(filePath.toString())
{
    Qt::ItemFlags itemFlags = flags();
    if (!isDir())
        itemFlags |= Qt::ItemIsUserCheckable;
    itemFlags &= ~(Qt::ItemIsEditable | Qt::ItemIsSelectable);
    setFlags(itemFlags);
}

const FilePath CheckableFileTreeItem::toFilePath() const
{
    return FilePath::fromString(text());
}

bool CheckableFileTreeItem::isFile() const
{
    return FilePath::fromString(text()).isFile();
}

bool CheckableFileTreeItem::isDir() const
{
    return FilePath::fromString(text()).isDir();
}

void CheckableFileTreeItem::setChecked(bool checked)
{
    this->checked = checked;
}

bool CheckableFileTreeItem::isChecked() const
{
    return this->checked;
}

} //QmlProjectManager
