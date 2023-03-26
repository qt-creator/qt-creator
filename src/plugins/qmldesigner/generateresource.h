// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once
#include <QTableWidget>

namespace QmlDesigner {
namespace GenerateResource {
struct ResourceFile
{
    QString fileName;
    bool inProject;
};
    void generateMenuEntry(QObject *parent);
    QStringList getFileList(const QList<ResourceFile> &);
    QTableWidget* createFilesTable(const QList<ResourceFile> &);
}
} // namespace QmlDesigner
