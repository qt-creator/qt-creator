// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangfileinfo.h"
#include "clangtoolsdiagnostic.h"
#include "clangtoolslogfilereader.h"
#include "clangtoolssettings.h"

#include <cppeditor/clangdiagnosticconfig.h>

#include <utils/environment.h>

namespace Tasking { class GroupItem; }

namespace ClangTools {
namespace Internal {

struct AnalyzeUnit
{
    AnalyzeUnit(const FileInfo &fileInfo, CppEditor::ClangToolType toolType)
        : file(fileInfo.file)
        , toolType(toolType)
    {}

    Utils::FilePath file;
    CppEditor::ClangToolType toolType;

};
using AnalyzeUnits = QList<AnalyzeUnit>;

struct AnalyzeInputData
{
    CppEditor::ClangToolType tool = CppEditor::ClangToolType::Tidy;
    RunSettings runSettings;
    CppEditor::ClangDiagnosticConfig config;
    Utils::FilePath outputDirPath;
    Utils::Environment environment;
    QString overlayFilePath = {};
    AcceptDiagsFromFilePath diagnosticsFilter = {};
};

struct AnalyzeOutputData
{
    bool success = true;
    Utils::FilePath fileToAnalyze;
    Utils::FilePath outputFilePath;
    Diagnostics diagnostics;
    CppEditor::ClangToolType toolType;
    QString errorMessage = {};
    QString errorDetails = {};
};

using AnalyzeSetupHandler = std::function<bool(const AnalyzeUnit &)>;
using AnalyzeOutputHandler = std::function<void(const AnalyzeOutputData &)>;

Tasking::GroupItem clangToolTask(CppEditor::ClangToolType toolType,
                                 const AnalyzeUnits &units,
                                 const AnalyzeInputData &input,
                                 const AnalyzeSetupHandler &setupHandler,
                                 const AnalyzeOutputHandler &outputHandler);

} // namespace Internal
} // namespace ClangTools
