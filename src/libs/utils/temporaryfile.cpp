// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "temporaryfile.h"

#include "filepath.h"
#include "qtcassert.h"
#include "temporarydirectory.h"

namespace Utils {

TemporaryFile::TemporaryFile(const QString &pattern) :
    QTemporaryFile(TemporaryDirectory::masterTemporaryDirectory()->path() + '/' + pattern)
{
    QTC_CHECK(!QFileInfo(pattern).isAbsolute());
}

TemporaryFile::~TemporaryFile() = default;

FilePath TemporaryFile::filePath() const
{
    return FilePath::fromString(QTemporaryFile::fileName());
}

QString TemporaryFile::fileName() const
{
    return QTemporaryFile::fileName();
}

} // namespace Utils
