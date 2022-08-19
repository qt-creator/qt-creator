// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor.h>

namespace CMakeProjectManager {
namespace Internal {

class CMakeEditorWidget;

class CMakeEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    void contextHelp(const HelpCallback &callback) const override;

    friend class CMakeEditorWidget;
};

class CMakeEditorFactory : public TextEditor::TextEditorFactory
{
public:
    CMakeEditorFactory();
};

} // namespace Internal
} // namespace CMakeProjectManager
