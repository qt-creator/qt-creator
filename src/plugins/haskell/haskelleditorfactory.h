// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor.h>

namespace Haskell::Internal {

class HaskellEditorFactory : public TextEditor::TextEditorFactory
{
public:
    HaskellEditorFactory();
};

} // Haskell::Internal
