// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppsourceprocessertesthelper.h"

#include <utils/filepath.h>

using namespace Utils;

namespace CppEditor::Tests::Internal::TestIncludePaths {

FilePath includeBaseDirectory()
{
    return FilePath::fromUserInput(QLatin1String(SRCDIR)
            + QLatin1String("/../../../tests/auto/cplusplus/preprocessor/data/include-data"));
}

FilePath globalQtCoreIncludePath()
{
    return includeBaseDirectory() / "QtCore";
}

FilePath globalIncludePath()
{
    return includeBaseDirectory() / "global";
}

FilePath directoryOfTestFile()
{
    return includeBaseDirectory() / "local";
}

FilePath testFilePath(const QString &fileName)
{
    return directoryOfTestFile() / fileName;
}

} // CppEditor::Tests::Internal::TestIncludePaths
