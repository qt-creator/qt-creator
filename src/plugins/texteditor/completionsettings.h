// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

namespace Utils { class QtcSettings; }
namespace TextEditor {

enum CaseSensitivity {
    CaseInsensitive,
    CaseSensitive,
    FirstLetterCaseSensitive
};

enum CompletionTrigger {
    ManualCompletion,     // Display proposal only when explicitly invoked by the user.
    TriggeredCompletion,  // When triggered by the user or upon contextual activation characters.
    AutomaticCompletion   // The above plus an automatic trigger when the editor is "idle".
};

/**
 * Settings that describe how the code completion behaves.
 */
class TEXTEDITOR_EXPORT CompletionSettings
{
public:
    void toSettings(Utils::QtcSettings *s) const;
    void fromSettings(Utils::QtcSettings *s);

    bool equals(const CompletionSettings &bs) const;

    friend bool operator==(const CompletionSettings &t1, const CompletionSettings &t2) { return t1.equals(t2); }
    friend bool operator!=(const CompletionSettings &t1, const CompletionSettings &t2) { return !t1.equals(t2); }

    CaseSensitivity m_caseSensitivity = CaseInsensitive;
    CompletionTrigger m_completionTrigger = AutomaticCompletion;
    int m_automaticProposalTimeoutInMs = 400;
    int m_characterThreshold = 3;
    bool m_autoInsertBrackets = true;
    bool m_surroundingAutoBrackets = true;
    bool m_autoInsertQuotes = true;
    bool m_surroundingAutoQuotes = true;
    bool m_partiallyComplete = true;
    bool m_spaceAfterFunctionName = false;
    bool m_autoSplitStrings = true;
    bool m_animateAutoComplete = true;
    bool m_highlightAutoComplete = true;
    bool m_skipAutoCompletedText = true;
    bool m_autoRemove = true;
    bool m_overwriteClosingChars = false;
};

} // namespace TextEditor
