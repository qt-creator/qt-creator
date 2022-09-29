// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/codeassist/keywordscompletionassist.h>

namespace CMakeProjectManager::Internal {

class CMakeFileCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
public:
    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const final;
};

} // CMakeProjectManager::Internal
