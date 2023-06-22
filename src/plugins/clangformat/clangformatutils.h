// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangformatsettings.h"

#include <utils/fileutils.h>
#include <utils/id.h>

#include <clang/Format/Format.h>

#include <QFile>

#include <fstream>

namespace TextEditor {
class ICodeStylePreferences;
class TabSettings;
}
namespace ProjectExplorer { class Project; }
namespace CppEditor { class CppCodeStyleSettings; }
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

void fromTabSettings(clang::format::FormatStyle &style, const TextEditor::TabSettings &settings);
void fromCppCodeStyleSettings(clang::format::FormatStyle &style,
                              const CppEditor::CppCodeStyleSettings &settings);

bool getProjectOverriddenSettings(const ProjectExplorer::Project *project);

void addQtcStatementMacros(clang::format::FormatStyle &style);
clang::format::FormatStyle qtcStyle();
clang::format::FormatStyle currentQtStyle(const TextEditor::ICodeStylePreferences *codeStyle);

Utils::FilePath filePathToCurrentSettings(const TextEditor::ICodeStylePreferences *codeStyle);
}
