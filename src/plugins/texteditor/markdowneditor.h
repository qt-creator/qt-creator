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
    Utils::Action m_emphasisAction;
    Utils::Action m_strongAction;
    Utils::Action m_inlineCodeAction;
    Utils::Action m_linkAction;
    Utils::Action m_toggleEditorAction;
    Utils::Action m_togglePreviewAction;
    Utils::Action m_swapAction;
};

} // TextEditor::Internal
