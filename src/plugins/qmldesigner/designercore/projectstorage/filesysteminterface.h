// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"

#include <utils/smallstringview.h>

namespace QmlDesigner {

class FileStatus;

class FileSystemInterface
{
public:
    virtual SourceIds directoryEntries(const QString &directoryPath) const = 0;
    virtual QStringList qmlFileNames(const QString &directoryPath) const = 0;
    virtual long long lastModified(SourceId sourceId) const = 0;
    virtual FileStatus fileStatus(SourceId sourceId) const = 0;
    virtual void remove(const SourceIds &sourceIds) = 0;
    virtual QString contentAsQString(const QString &filePath) const = 0;

protected:
    ~FileSystemInterface() = default;
};
} // namespace QmlDesigner
