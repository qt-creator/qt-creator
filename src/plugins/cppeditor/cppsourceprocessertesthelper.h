// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Utils { class FilePath; }

namespace CppEditor::Tests::Internal::TestIncludePaths {

Utils::FilePath includeBaseDirectory();
Utils::FilePath globalQtCoreIncludePath();
Utils::FilePath globalIncludePath();
Utils::FilePath directoryOfTestFile();
Utils::FilePath testFilePath(const QString &fileName = QLatin1String("file.cpp"));

} // CppEditor::Tests::Internal::TestIncludePaths
