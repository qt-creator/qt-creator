// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CMAKEGENERATORDIALOGTREEMODEL_H
#define CMAKEGENERATORDIALOGTREEMODEL_H

#include "checkablefiletreeitem.h"

#include <QFileIconProvider>
#include <QStandardItemModel>

#include <utils/fileutils.h>

namespace QmlProjectManager {
namespace GenerateCmake {

class CMakeGeneratorDialogTreeModel : public QStandardItemModel
{
    Q_OBJECT

public:
    CMakeGeneratorDialogTreeModel(const Utils::FilePath &rootDir,
                                  const Utils::FilePaths &files, QObject *parent = nullptr);
    ~CMakeGeneratorDialogTreeModel();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    const QList<CheckableFileTreeItem*> items() const;
    const QList<CheckableFileTreeItem*> checkedItems() const;
    const CheckableFileTreeItem* constNodeForIndex(const QModelIndex &index) const;
    CheckableFileTreeItem* nodeForIndex(const QModelIndex &index);

signals:
    void checkedStateChanged(CheckableFileTreeItem *item);

protected:
    bool checkedByDefault(const Utils::FilePath &file) const;
    Utils::FilePath rootDir;

private:
    void createNodes(const Utils::FilePaths &candidates, QStandardItem *parent);

    QFileIconProvider* m_icons;
};

} //GenerateCmake
} //QmlProjectManager


#endif // CMAKEGENERATORDIALOGTREEMODEL_H
