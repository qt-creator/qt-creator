// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "temporaryfile.h"

#include "filepath.h"
#include "qtcassert.h"
#include "temporarydirectory.h"

namespace Utils {

// TemporaryFilePath

class TemporaryFilePathPrivate
{
public:
    FilePath templatePath;
    FilePath filePath;
    bool autoRemove = true;
    bool dir = false;
};

Result<std::unique_ptr<TemporaryFilePath>> TemporaryFilePath::create(
    const FilePath &templatePath, bool directory)
{
    Result<FilePath> result = directory ? templatePath.createTempDir()
                                        : templatePath.createTempFile();
    if (!result)
        return ResultError(result.error());
    return std::unique_ptr<TemporaryFilePath>(new TemporaryFilePath(templatePath, *result, directory));
}

TemporaryFilePath::TemporaryFilePath(
    const FilePath &templatePath, const FilePath &filePath, bool directory)
    : d(std::make_unique<TemporaryFilePathPrivate>())
{
    d->templatePath = templatePath;
    d->filePath = filePath;
    d->dir = directory;
}

TemporaryFilePath::~TemporaryFilePath()
{
    if (d->autoRemove) {
        if (d->dir)
            d->filePath.removeRecursively();
        else
            d->filePath.removeFile();
    }
}

void TemporaryFilePath::setAutoRemove(bool autoRemove)
{
    d->autoRemove = autoRemove;
}

bool TemporaryFilePath::autoRemove() const
{
    return d->autoRemove;
}

FilePath TemporaryFilePath::templatePath() const
{
    return d->templatePath;
}

FilePath TemporaryFilePath::filePath() const
{
    return d->filePath;
}

// TemporaryFilePath

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
