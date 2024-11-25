// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "compileroptionsbuilder.h"
#include "cppeditor_global.h"
#include "projectinfo.h"

#include <utils/expected.h>
#include <utils/filepath.h>

#include <QPromise>
#include <QStringList>

#include <functional>

namespace CppEditor {
class ClangDiagnosticConfig;

using GenerateCompilationDbResult = Utils::expected_str<Utils::FilePath>;
using GetOptionsBuilder = std::function<CompilerOptionsBuilder(const ProjectPart &)>;
enum class CompilationDbPurpose { Project, CodeModel, Analysis };

QJsonArray CPPEDITOR_EXPORT fullProjectPartOptions(
    const CppEditor::CompilerOptionsBuilder &optionsBuilder,
    const QStringList &projectOptions);

QJsonArray CPPEDITOR_EXPORT clangOptionsForFile(
    const ProjectFile &file,
    const ProjectPart &projectPart,
    const QJsonArray &generalOptions,
    UsePrecompiledHeaders usePch,
    bool clStyle);

void CPPEDITOR_EXPORT generateCompilationDB(
    QPromise<GenerateCompilationDbResult> &promise,
    const QList<ProjectInfo::ConstPtr> &projectInfoList,
    const Utils::FilePath &baseDir,
    CompilationDbPurpose purpose,
    const QStringList &projectOptions,
    const GetOptionsBuilder &getOptionsBuilder);

} // namespace CppEditor
