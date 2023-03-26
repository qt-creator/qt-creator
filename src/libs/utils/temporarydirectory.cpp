// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "temporarydirectory.h"

#include "filepath.h"
#include "qtcassert.h"

#include <QCoreApplication>

namespace Utils {

static QTemporaryDir* m_masterTemporaryDir = nullptr;

static void cleanupMasterTemporaryDir()
{
    delete m_masterTemporaryDir;
    m_masterTemporaryDir = nullptr;
}

TemporaryDirectory::TemporaryDirectory(const QString &pattern) :
    QTemporaryDir(m_masterTemporaryDir->path() + '/' + pattern)
{
    QTC_CHECK(!QFileInfo(pattern).isAbsolute());
}

QTemporaryDir *TemporaryDirectory::masterTemporaryDirectory()
{
    return m_masterTemporaryDir;
}

void TemporaryDirectory::setMasterTemporaryDirectory(const QString &pattern)
{
    if (m_masterTemporaryDir)
        cleanupMasterTemporaryDir();
    else
        qAddPostRoutine(cleanupMasterTemporaryDir);
    m_masterTemporaryDir = new QTemporaryDir(pattern);
}

QString TemporaryDirectory::masterDirectoryPath()
{
    return m_masterTemporaryDir->path();
}

FilePath TemporaryDirectory::masterDirectoryFilePath()
{
    return FilePath::fromString(TemporaryDirectory::masterDirectoryPath());
}

FilePath TemporaryDirectory::path() const
{
    return FilePath::fromString(QTemporaryDir::path());
}

FilePath TemporaryDirectory::filePath(const QString &fileName) const
{
    return FilePath::fromString(QTemporaryDir::filePath(fileName));
}

} // namespace Utils
