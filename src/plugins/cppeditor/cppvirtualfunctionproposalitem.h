// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <texteditor/texteditor.h>
#include <texteditor/codeassist/assistproposalitem.h>

namespace CppEditor {

class CPPEDITOR_EXPORT VirtualFunctionProposalItem final : public TextEditor::AssistProposalItem
{
public:
    VirtualFunctionProposalItem(const Utils::Link &link,
                                bool openInSplit = true);
    ~VirtualFunctionProposalItem() noexcept override = default;
    void apply(TextEditor::TextDocumentManipulatorInterface &manipulator,
               int basePosition) const override;
    Utils::Link link() const { return m_link; } // Exposed for tests

private:
    Utils::Link m_link;
    bool m_openInSplit;
};

} // namespace CppEditor
