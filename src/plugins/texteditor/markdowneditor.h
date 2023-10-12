// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/ieditorfactory.h>

#include <texteditor/texteditoractionhandler.h>

#include <QAction>

namespace TextEditor::Internal {

class MarkdownEditorFactory final : public Core::IEditorFactory
{
public:
    MarkdownEditorFactory();

private:
    TextEditor::TextEditorActionHandler m_actionHandler;
    QAction m_emphasisAction;
    QAction m_strongAction;
    QAction m_inlineCodeAction;
    QAction m_linkAction;
    QAction m_toggleEditorAction;
    QAction m_togglePreviewAction;
    QAction m_swapAction;
};

} // TextEditor::Internal
