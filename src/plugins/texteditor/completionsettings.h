// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/aspects.h>

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
class TEXTEDITOR_EXPORT CompletionSettings : public Utils::AspectContainer
{
public:
    CompletionSettings();

    Utils::TypedSelectionAspect<CaseSensitivity> caseSensitivity{this};
    Utils::TypedSelectionAspect<CompletionTrigger> completionTrigger{this};
    Utils::IntegerAspect automaticProposalTimeoutInMs{this};
    Utils::IntegerAspect characterThreshold{this};
    Utils::BoolAspect autoInsertBrackets{this};
    Utils::BoolAspect surroundingAutoBrackets{this};
    Utils::BoolAspect autoInsertQuotes{this};
    Utils::BoolAspect surroundingAutoQuotes{this};
    Utils::BoolAspect partiallyComplete{this};
    Utils::BoolAspect spaceAfterFunctionName{this};
    Utils::BoolAspect autoSplitStrings{this};
    Utils::BoolAspect animateAutoComplete{this};
    Utils::BoolAspect highlightAutoComplete{this};
    Utils::BoolAspect skipAutoCompletedText{this};
    Utils::BoolAspect autoRemove{this};
    Utils::BoolAspect overwriteClosingChars{this};
};

TEXTEDITOR_EXPORT CompletionSettings &globalCompletionSettings();

namespace Internal { void setupCompletionSettings(); }

} // namespace TextEditor
