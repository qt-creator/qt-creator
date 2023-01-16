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

// Creates the style for the current project or the global style if needed.
void createStyleFileIfNeeded(bool isGlobal);

QString currentProjectUniqueId();

std::string readFile(const QString &path);

bool getProjectUseGlobalSettings(const ProjectExplorer::Project *project);

bool getProjectOverriddenSettings(const ProjectExplorer::Project *project);
bool getCurrentOverriddenSettings(const Utils::FilePath &filePath);

ClangFormatSettings::Mode getProjectIndentationOrFormattingSettings(
    const ProjectExplorer::Project *project);
ClangFormatSettings::Mode getCurrentIndentationOrFormattingSettings(const Utils::FilePath &filePath);

// Is the style from the matching .clang-format file or global one if it's not found.
QString configForFile(Utils::FilePath fileName);
void saveStyleToFile(clang::format::FormatStyle style, Utils::FilePath filePath);

void addQtcStatementMacros(clang::format::FormatStyle &style);
clang::format::FormatStyle qtcStyle();

Utils::FilePath filePathToCurrentSettings(const TextEditor::ICodeStylePreferences *codeStyle);
}
