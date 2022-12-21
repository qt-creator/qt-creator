// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

struct FileData
{
    FileData(const QString &f, const QString &c)
    { filename = f; content = c; }

    QString filename;
    QString content;
};

typedef QList<FileData> FileDataList;

FileDataList splitDiffToFiles(const QString &data);
