// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "requests/getcompletions.h"

#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>

namespace Copilot::Internal {

class CopilotSuggestion final : public TextEditor::TextSuggestion
{
public:
    CopilotSuggestion(const QList<Completion> &completions,
                      QTextDocument *origin,
                      int currentCompletion = 0);

    bool apply() final;
    bool applyWord(TextEditor::TextEditorWidget *widget) final;
    void reset() final;
    int position() final;

    const QList<Completion> &completions() const { return m_completions; }
    int currentCompletion() const { return m_currentCompletion; }

private:
    QList<Completion> m_completions;
    int m_currentCompletion = 0;
    QTextCursor m_start;
};
} // namespace Copilot::Internal
