// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace TextEditor {
class ICodeStylePreferences;
class CodeStyleEditorWidget;
} // namespace TextEditor

namespace ProjectExplorer { class Project; }

namespace ClangFormat {

TextEditor::CodeStyleEditorWidget *createClangFormatConfigWidget(
    TextEditor::ICodeStylePreferences *codeStyle,
    ProjectExplorer::Project *project,
    QWidget *parent);

} // ClangFormat
