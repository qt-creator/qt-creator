// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangfileinfo.h"

#include <cppeditor/clangdiagnosticconfig.h>

#include <utils/environment.h>

namespace Utils::Tasking { class TaskItem; }

namespace ClangTools {
namespace Internal {

struct AnalyzeUnit
{
    AnalyzeUnit(const FileInfo &fileInfo,
                const Utils::FilePath &clangResourceDir,
                const QString &clangVersion);

    Utils::FilePath file;
    QStringList arguments; // without file itself and "-o somePath"
};
using AnalyzeUnits = QList<AnalyzeUnit>;

struct AnalyzeInputData
{
    CppEditor::ClangToolType tool = CppEditor::ClangToolType::Tidy;
    CppEditor::ClangDiagnosticConfig config;
    Utils::FilePath outputDirPath;
    Utils::Environment environment;
    AnalyzeUnit unit;
    QString overlayFilePath = {};
};

struct AnalyzeOutputData
{
    bool success = true;
    Utils::FilePath fileToAnalyze;
    Utils::FilePath outputFilePath;
    CppEditor::ClangToolType toolType;
    QString errorMessage = {};
    QString errorDetails = {};
};

using AnalyzeSetupHandler = std::function<bool()>;
using AnalyzeOutputHandler = std::function<void(const AnalyzeOutputData &)>;

Utils::Tasking::TaskItem clangToolTask(const AnalyzeInputData &input,
                                       const AnalyzeSetupHandler &setupHandler,
                                       const AnalyzeOutputHandler &outputHandler);

} // namespace Internal
} // namespace ClangTools
