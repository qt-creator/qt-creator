// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testdatadir.h"

#include <QDir>
#include <QFileInfo>
#include <QTest>

#include <utils/filepath.h>

using namespace Utils;

namespace Core::Tests {

TestDataDir::TestDataDir(const QString &directory)
    : m_directory(directory)
{
    QFileInfo fi(m_directory);
    QVERIFY(fi.exists());
    QVERIFY(fi.isDir());
}

QString TestDataDir::file(const QString &fileName) const
{
    return directory() + QLatin1Char('/') + fileName;
}

FilePath TestDataDir::filePath(const QString &fileName) const
{
    return FilePath::fromString(directory()) / fileName;
}

QString TestDataDir::path() const
{
    return m_directory;
}

QString TestDataDir::directory(const QString &subdir, bool clean) const
{
    QString path = m_directory;
    if (!subdir.isEmpty())
        path += QLatin1Char('/') + subdir;
    if (clean)
        path = QDir::cleanPath(path);
    return path;
}

} // Core::Tests
