// Copyright (C) 2016 Jan Dalheimer <jan@dalheimer.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"

#include <texteditor/textindenter.h>

namespace CMakeProjectManager::Internal {

class CMAKE_EXPORT CMakeIndenter : public TextEditor::TextIndenter
{
public:
    explicit CMakeIndenter(QTextDocument *doc);
    bool isElectricCharacter(const QChar &ch) const override;

    int indentFor(const QTextBlock &block,
                  const TextEditor::TabSettings &tabSettings,
                  int cursorPositionInEditor = -1) override;
};

} // CMakeProjectManager::Internal
