// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangformatsettings.h"

#include <utils/filepath.h>
#include <utils/id.h>

#include <clang/Format/Format.h>

#include <QFile>

namespace TextEditor {
class ICodeStylePreferences;
class TabSettings;
}
namespace ProjectExplorer { class Project; }
namespace CppEditor { class CppCodeStyleSettings; }

namespace ClangFormat {

QString projectUniqueId(ProjectExplorer::Project *project);

bool getProjectUseGlobalSettings(const ProjectExplorer::Project *project);

bool getProjectCustomSettings(const ProjectExplorer::Project *project);
bool getCurrentCustomSettings(const Utils::FilePath &filePath);

ClangFormatSettings::Mode getProjectIndentationOrFormattingSettings(
    const ProjectExplorer::Project *project);
ClangFormatSettings::Mode getCurrentIndentationOrFormattingSettings(const Utils::FilePath &filePath);

Utils::FilePath configForFile(const Utils::FilePath &fileName);
Utils::FilePath findConfig(const Utils::FilePath &fileName);

void fromTabSettings(clang::format::FormatStyle &style, const TextEditor::TabSettings &settings);
void fromCppCodeStyleSettings(clang::format::FormatStyle &style,
                              const CppEditor::CppCodeStyleSettings &settings);

bool getProjectCustomSettings(const ProjectExplorer::Project *project);

void addQtcStatementMacros(clang::format::FormatStyle &style);
clang::format::FormatStyle qtcStyle();
clang::format::FormatStyle currentQtStyle(const TextEditor::ICodeStylePreferences *codeStyle);

Utils::FilePath filePathToCurrentSettings(const TextEditor::ICodeStylePreferences *codeStyle);

Utils::expected_str<void> parseConfigurationContent(const std::string &fileContent,
                                                    clang::format::FormatStyle &style,
                                                    bool allowUnknownOptions = false);
Utils::expected_str<void> parseConfigurationFile(const Utils::FilePath &filePath,
                                                 clang::format::FormatStyle &style);

} // ClangFormat
