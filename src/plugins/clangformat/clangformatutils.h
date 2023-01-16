// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangformatsettings.h"

#include <utils/fileutils.h>
#include <utils/id.h>

#include <clang/Format/Format.h>

#include <QFile>

#include <fstream>

namespace TextEditor { class ICodeStylePreferences; }
namespace ProjectExplorer { class Project; }
namespace ClangFormat {

QString projectUniqueId(ProjectExplorer::Project *project);

bool getProjectUseGlobalSettings(const ProjectExplorer::Project *project);

bool getProjectOverriddenSettings(const ProjectExplorer::Project *project);
bool getCurrentOverriddenSettings(const Utils::FilePath &filePath);

ClangFormatSettings::Mode getProjectIndentationOrFormattingSettings(
    const ProjectExplorer::Project *project);
ClangFormatSettings::Mode getCurrentIndentationOrFormattingSettings(const Utils::FilePath &filePath);

Utils::FilePath configForFile(const Utils::FilePath &fileName);
Utils::FilePath findConfig(const Utils::FilePath &fileName);

bool getProjectOverriddenSettings(const ProjectExplorer::Project *project);

void addQtcStatementMacros(clang::format::FormatStyle &style);
clang::format::FormatStyle qtcStyle();

Utils::FilePath filePathToCurrentSettings(const TextEditor::ICodeStylePreferences *codeStyle);
}
