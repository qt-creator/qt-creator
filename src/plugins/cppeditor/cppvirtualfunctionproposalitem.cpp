// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppvirtualfunctionproposalitem.h"

#include "cppeditorconstants.h"

#include <coreplugin/editormanager/editormanager.h>

namespace CppEditor {

VirtualFunctionProposalItem::VirtualFunctionProposalItem(
        const Utils::Link &link, bool openInSplit)
    : m_link(link), m_openInSplit(openInSplit)
{
}

void VirtualFunctionProposalItem::apply(TextEditor::TextDocumentManipulatorInterface &,
                                        int) const
{
    if (!m_link.hasValidTarget())
        return;

    Core::EditorManager::OpenEditorFlags flags = Core::EditorManager::NoFlags;
    if (m_openInSplit)
        flags |= Core::EditorManager::OpenInOtherSplit;
    Core::EditorManager::openEditorAt(m_link, CppEditor::Constants::CPPEDITOR_ID, flags);
}

} // namespace CppEditor
