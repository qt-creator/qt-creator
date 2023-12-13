// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace CppEditor { class CppCodeStyleWidget; }
namespace TextEditor { class ICodeStylePreferences; }
namespace ProjectExplorer { class Project; }

namespace ClangFormat {

CppEditor::CppCodeStyleWidget *createClangFormatConfigWidget(
    TextEditor::ICodeStylePreferences *codeStyle,
    ProjectExplorer::Project *project,
    QWidget *parent);

} // ClangFormat
