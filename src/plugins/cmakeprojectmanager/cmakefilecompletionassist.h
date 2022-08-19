// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/codeassist/keywordscompletionassist.h>

namespace CMakeProjectManager {
namespace Internal {

class CMakeFileCompletionAssist : public TextEditor::KeywordsCompletionAssistProcessor
{
public:
    CMakeFileCompletionAssist();

    // IAssistProcessor interface
    TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *interface) override;
};

class CMakeFileCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const override;
};

} // Internal
} // CMakeProjectManager
