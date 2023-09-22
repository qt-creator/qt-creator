// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "completionsettings.h"

#include <utils/qtcsettings.h>

using namespace Utils;

namespace TextEditor {

const char settingsGroup[]               = "CppTools/Completion";
const char caseSensitivityKey[]          = "CaseSensitivity";
const char completionTriggerKey[]        = "CompletionTrigger";
const char automaticProposalTimeoutKey[] = "AutomaticProposalTimeout";
const char characterThresholdKey[]       = "CharacterThreshold";
const char autoInsertBracesKey[]         = "AutoInsertBraces";
const char surroundingAutoBracketsKey[]  = "SurroundingAutoBrackets";
const char autoInsertQuotesKey[]         = "AutoInsertQuotes";
const char surroundingAutoQuotesKey[]    = "SurroundingAutoQuotes";
const char partiallyCompleteKey[]        = "PartiallyComplete";
const char spaceAfterFunctionNameKey[]   = "SpaceAfterFunctionName";
const char autoSplitStringsKey[]         = "AutoSplitStrings";
const char animateAutoCompleteKey[]      = "AnimateAutoComplete";
const char highlightAutoCompleteKey[]    = "HighlightAutoComplete";
const char skipAutoCompleteKey[]         = "SkipAutoComplete";
const char autoRemoveKey[]               = "AutoRemove";
const char overwriteClosingCharsKey[]    = "OverwriteClosingChars";

void CompletionSettings::toSettings(QtcSettings *s) const
{
    s->beginGroup(settingsGroup);
    s->setValue(caseSensitivityKey, (int) m_caseSensitivity);
    s->setValue(completionTriggerKey, (int) m_completionTrigger);
    s->setValue(automaticProposalTimeoutKey, m_automaticProposalTimeoutInMs);
    s->setValue(characterThresholdKey, m_characterThreshold);
    s->setValue(autoInsertBracesKey, m_autoInsertBrackets);
    s->setValue(surroundingAutoBracketsKey, m_surroundingAutoBrackets);
    s->setValue(autoInsertQuotesKey, m_autoInsertQuotes);
    s->setValue(surroundingAutoQuotesKey, m_surroundingAutoQuotes);
    s->setValue(partiallyCompleteKey, m_partiallyComplete);
    s->setValue(spaceAfterFunctionNameKey, m_spaceAfterFunctionName);
    s->setValue(autoSplitStringsKey, m_autoSplitStrings);
    s->setValue(animateAutoCompleteKey, m_animateAutoComplete);
    s->setValue(highlightAutoCompleteKey, m_highlightAutoComplete);
    s->setValue(skipAutoCompleteKey, m_skipAutoCompletedText);
    s->setValue(autoRemoveKey, m_autoRemove);
    s->setValue(overwriteClosingCharsKey, m_overwriteClosingChars);
    s->endGroup();
}

void CompletionSettings::fromSettings(QtcSettings *s)
{
    *this = CompletionSettings(); // Assign defaults

    s->beginGroup(settingsGroup);
    m_caseSensitivity = (CaseSensitivity)
            s->value(caseSensitivityKey, m_caseSensitivity).toInt();
    m_completionTrigger = (CompletionTrigger)
            s->value(completionTriggerKey, m_completionTrigger).toInt();
    m_automaticProposalTimeoutInMs =
            s->value(automaticProposalTimeoutKey, m_automaticProposalTimeoutInMs).toInt();
    m_characterThreshold =
            s->value(characterThresholdKey, m_characterThreshold).toInt();
    m_autoInsertBrackets =
            s->value(autoInsertBracesKey, m_autoInsertBrackets).toBool();
    m_surroundingAutoBrackets =
            s->value(surroundingAutoBracketsKey, m_surroundingAutoBrackets).toBool();
    m_autoInsertQuotes =
            s->value(autoInsertQuotesKey, m_autoInsertQuotes).toBool();
    m_surroundingAutoQuotes =
            s->value(surroundingAutoQuotesKey, m_surroundingAutoQuotes).toBool();
    m_partiallyComplete =
            s->value(partiallyCompleteKey, m_partiallyComplete).toBool();
    m_spaceAfterFunctionName =
            s->value(spaceAfterFunctionNameKey, m_spaceAfterFunctionName).toBool();
    m_autoSplitStrings =
            s->value(autoSplitStringsKey, m_autoSplitStrings).toBool();
    m_animateAutoComplete =
            s->value(animateAutoCompleteKey, m_animateAutoComplete).toBool();
    m_highlightAutoComplete =
            s->value(highlightAutoCompleteKey, m_highlightAutoComplete).toBool();
    m_skipAutoCompletedText =
            s->value(skipAutoCompleteKey, m_skipAutoCompletedText).toBool();
    m_autoRemove =
            s->value(autoRemoveKey, m_autoRemove).toBool();
    m_overwriteClosingChars =
            s->value(overwriteClosingCharsKey, m_overwriteClosingChars).toBool();
    s->endGroup();
}

bool CompletionSettings::equals(const CompletionSettings &cs) const
{
    return m_caseSensitivity                == cs.m_caseSensitivity
        && m_completionTrigger              == cs.m_completionTrigger
        && m_automaticProposalTimeoutInMs   == cs.m_automaticProposalTimeoutInMs
        && m_characterThreshold             == cs.m_characterThreshold
        && m_autoInsertBrackets             == cs.m_autoInsertBrackets
        && m_surroundingAutoBrackets        == cs.m_surroundingAutoBrackets
        && m_autoInsertQuotes               == cs.m_autoInsertQuotes
        && m_surroundingAutoQuotes          == cs.m_surroundingAutoQuotes
        && m_partiallyComplete              == cs.m_partiallyComplete
        && m_spaceAfterFunctionName         == cs.m_spaceAfterFunctionName
        && m_autoSplitStrings               == cs.m_autoSplitStrings
        && m_animateAutoComplete            == cs.m_animateAutoComplete
        && m_highlightAutoComplete          == cs.m_highlightAutoComplete
        && m_skipAutoCompletedText          == cs.m_skipAutoCompletedText
        && m_autoRemove                     == cs.m_autoRemove
        && m_overwriteClosingChars          == cs.m_overwriteClosingChars
        ;
}

} // TextEditor
