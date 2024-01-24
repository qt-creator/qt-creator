// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/ieditorfactory.h>

#include <texteditor/texteditoractionhandler.h>

#include <utils/parameteraction.h>

namespace TextEditor::Internal {

class MarkdownEditorFactory final : public Core::IEditorFactory
{
public:
    MarkdownEditorFactory();

private:
    TextEditor::TextEditorActionHandler m_actionHandler;
    Utils::ParameterAction m_emphasisAction;
    Utils::ParameterAction m_strongAction;
    Utils::ParameterAction m_inlineCodeAction;
    Utils::ParameterAction m_linkAction;
    Utils::ParameterAction m_toggleEditorAction;
    Utils::ParameterAction m_togglePreviewAction;
    Utils::ParameterAction m_swapAction;
};

} // TextEditor::Internal
