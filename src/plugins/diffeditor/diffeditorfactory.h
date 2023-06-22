// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/ieditorfactory.h>

#include <texteditor/texteditoractionhandler.h>

namespace DiffEditor::Internal {

class DiffEditorFactory : public Core::IEditorFactory
{
public:
    DiffEditorFactory();

private:
    TextEditor::TextEditorActionHandler descriptionHandler;
    TextEditor::TextEditorActionHandler unifiedHandler;
    TextEditor::TextEditorActionHandler leftHandler;
    TextEditor::TextEditorActionHandler rightHandler;
};

} // namespace DiffEditor::Internal
