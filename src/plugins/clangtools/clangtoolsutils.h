// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/clangdiagnosticconfig.h>

#include <utils/id.h>

#include <QtGlobal>

#include <optional>

namespace CppEditor { class ClangDiagnosticConfigsModel; }
namespace Debugger { class DiagnosticLocation; }
namespace Utils { class FilePath; }

namespace ClangTools {
namespace Internal {
class RunSettings;

QString clangTidyDocUrl(const QString &check);
QString clazyDocUrl(const QString &check);

class Diagnostic;

enum class FixitStatus {
    NotAvailable,
    NotScheduled,
    Scheduled,
    Applied,
    FailedToApply,
    Invalidated,
};

QString createDiagnosticToolTipString(const Diagnostic &diagnostic,
                                      std::optional<FixitStatus> status = std::nullopt,
                                      bool showSteps = true);

CppEditor::ClangDiagnosticConfig builtinConfig();

QString createFullLocationString(const Debugger::DiagnosticLocation &location);

QString hintAboutBuildBeforeAnalysis();
void showHintAboutBuildBeforeAnalysis();

Utils::FilePath toolShippedExecutable(CppEditor::ClangToolType tool);
Utils::FilePath toolExecutable(CppEditor::ClangToolType tool);
Utils::FilePath toolFallbackExecutable(CppEditor::ClangToolType tool);
QString clangToolName(CppEditor::ClangToolType tool);
bool toolEnabled(CppEditor::ClangToolType type, const CppEditor::ClangDiagnosticConfig &config,
                 const RunSettings &runSettings);

bool isVFSOverlaySupported(const Utils::FilePath &executable);

Utils::FilePath fullPath(const Utils::FilePath &executable);

QString documentationUrl(const QString &checkName);

CppEditor::ClangDiagnosticConfigsModel diagnosticConfigsModel();
CppEditor::ClangDiagnosticConfigsModel diagnosticConfigsModel(
    const CppEditor::ClangDiagnosticConfigs &customConfigs);

CppEditor::ClangDiagnosticConfig diagnosticConfig(const Utils::Id &diagConfigId);

QStringList extraClangToolsPrependOptions();
QStringList extraClangToolsAppendOptions();

inline Utils::Id taskCategory() { return "ClangTools"; }

} // namespace Internal
} // namespace ClangTools
