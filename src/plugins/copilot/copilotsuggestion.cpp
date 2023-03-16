// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "copilotsuggestion.h"

namespace Copilot::Internal {

CopilotSuggestion::CopilotSuggestion(const QList<Completion> &completions,
                                     QTextDocument *origin,
                                     int currentCompletion)
    : m_completions(completions)
    , m_currentCompletion(currentCompletion)
{
    const Completion completion = completions.value(currentCompletion);
    document()->setPlainText(completion.text());
    m_start = completion.position().toTextCursor(origin);
    m_start.setKeepPositionOnInsert(true);
    setCurrentPosition(m_start.position());
}

bool CopilotSuggestion::apply()
{
    reset();
    const Completion completion = m_completions.value(m_currentCompletion);
    QTextCursor cursor = completion.range().toSelection(m_start.document());
    cursor.insertText(completion.text());
    return true;
}

void CopilotSuggestion::reset()
{
    m_start.removeSelectedText();
}

int CopilotSuggestion::position()
{
    return m_start.position();
}

} // namespace Copilot::Internal

