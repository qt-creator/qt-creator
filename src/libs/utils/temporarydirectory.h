// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QTemporaryDir>

namespace Utils {

class FilePath;

class QTCREATOR_UTILS_EXPORT TemporaryDirectory : public QTemporaryDir
{
public:
    explicit TemporaryDirectory(const QString &pattern);

    static QTemporaryDir *masterTemporaryDirectory();
    static void setMasterTemporaryDirectory(const QString &pattern);
    static QString masterDirectoryPath();
    static FilePath masterDirectoryFilePath();

    FilePath path() const;
    FilePath filePath(const QString &fileName) const;
};

} // namespace Utils
