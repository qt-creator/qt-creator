// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "cppsourceprocessertesthelper.h"

#include <QDir>

namespace CppEditor::Tests::Internal {

QString TestIncludePaths::includeBaseDirectory()
{
    return QLatin1String(SRCDIR)
            + QLatin1String("/../../../tests/auto/cplusplus/preprocessor/data/include-data");
}

QString TestIncludePaths::globalQtCoreIncludePath()
{
    return QDir::cleanPath(includeBaseDirectory() + QLatin1String("/QtCore"));
}

QString TestIncludePaths::globalIncludePath()
{
    return QDir::cleanPath(includeBaseDirectory() + QLatin1String("/global"));
}

QString TestIncludePaths::directoryOfTestFile()
{
    return QDir::cleanPath(includeBaseDirectory() + QLatin1String("/local"));
}

QString TestIncludePaths::testFilePath(const QString &fileName)
{
    return directoryOfTestFile() + QLatin1Char('/') + fileName;
}

} // namespace CppEditor::Tests::Internal
