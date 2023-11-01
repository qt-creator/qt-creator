// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/codeassist/completionassistprovider.h>

namespace CMakeProjectManager::Internal {

class CMakeFileCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
public:
    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const final;
    int activationCharSequenceLength() const final;
    bool isActivationCharSequence(const QString &sequence) const final;
};

} // CMakeProjectManager::Internal
