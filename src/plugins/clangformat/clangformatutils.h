// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>
#include <utils/id.h>

#include <clang/Format/Format.h>

#include <QFile>

#include <fstream>

namespace TextEditor { class ICodeStylePreferences; }
namespace ClangFormat {

// Creates the style for the current project or the global style if needed.
void createStyleFileIfNeeded(bool isGlobal);

QString currentProjectUniqueId();

std::string currentProjectConfigText();
std::string currentGlobalConfigText();

clang::format::FormatStyle currentProjectStyle();
clang::format::FormatStyle currentGlobalStyle();
std::string readFile(const QString &path);

// Is the style from the matching .clang-format file or global one if it's not found.
QString configForFile(Utils::FilePath fileName);
clang::format::FormatStyle styleForFile(Utils::FilePath fileName);
void saveStyleToFile(clang::format::FormatStyle style, Utils::FilePath filePath);

void addQtcStatementMacros(clang::format::FormatStyle &style);
clang::format::FormatStyle qtcStyle();

Utils::FilePath filePathToCurrentSettings(const TextEditor::ICodeStylePreferences *codeStyle);
}
