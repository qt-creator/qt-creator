// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CHECKABLEFILETREEITEM_H
#define CHECKABLEFILETREEITEM_H

#include <utils/fileutils.h>

#include <QStandardItem>

namespace QmlProjectManager {

class CheckableFileTreeItem : public QStandardItem
{
public:
    explicit CheckableFileTreeItem(const Utils::FilePath &text = Utils::FilePath());

    const Utils::FilePath toFilePath() const;
    bool isFile() const;
    bool isDir() const;

    bool isChecked() const;
    void setChecked(bool checked);

private:
    bool checked;
};

} //QmlProjectManager

#endif // CHECKABLEFILETREEITEM_H
