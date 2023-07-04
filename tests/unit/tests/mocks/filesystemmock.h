// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <projectstorage/filestatus.h>
#include <projectstorage/filesysteminterface.h>

class FileSystemMock : public QmlDesigner::FileSystemInterface
{
public:
    MOCK_METHOD(QmlDesigner::SourceIds,
                directoryEntries,
                (const QString &directoryPath),
                (const, override));
    MOCK_METHOD(QStringList, qmlFileNames, (const QString &directoryPath), (const, override));
    MOCK_METHOD(long long, lastModified, (QmlDesigner::SourceId sourceId), (const, override));
    MOCK_METHOD(QmlDesigner::FileStatus, fileStatus, (QmlDesigner::SourceId sourceId), (const, override));
    MOCK_METHOD(void, remove, (const QmlDesigner::SourceIds &sourceIds), (override));
    MOCK_METHOD(QString, contentAsQString, (const QString &filePath), (const, override));
};
