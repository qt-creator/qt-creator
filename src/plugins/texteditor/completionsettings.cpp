/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "completionsettings.h"

#include <QSettings>

static const char settingsGroup[]               = "CppTools/Completion";
static const char caseSensitivityKey[]          = "CaseSensitivity";
static const char completionTriggerKey[]        = "CompletionTrigger";
static const char automaticProposalTimeoutKey[] = "AutomaticProposalTimeout";
static const char autoInsertBracesKey[]         = "AutoInsertBraces";
static const char surroundingAutoBracketsKey[]  = "SurroundingAutoBrackets";
static const char autoInsertQuotesKey[]         = "AutoInsertQuotes";
static const char surroundingAutoQuotesKey[]    = "SurroundingAutoQuotes";
static const char partiallyCompleteKey[]        = "PartiallyComplete";
static const char spaceAfterFunctionNameKey[]   = "SpaceAfterFunctionName";
static const char autoSplitStringsKey[]         = "AutoSplitStrings";
static const char animateAutoCompleteKey[]      = "AnimateAutoComplete";
static const char highlightAutoCompleteKey[]    = "HighlightAutoComplete";
static const char skipAutoCompleteKey[]         = "SkipAutoComplete";
static const char autoRemoveKey[]               = "AutoRemove";

using namespace TextEditor;

void CompletionSettings::toSettings(QSettings *s) const
{
    s->beginGroup(settingsGroup);
    s->setValue(caseSensitivityKey, (int) m_caseSensitivity);
    s->setValue(completionTriggerKey, (int) m_completionTrigger);
    s->setValue(automaticProposalTimeoutKey, m_automaticProposalTimeoutInMs);
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
    s->endGroup();
}

void CompletionSettings::fromSettings(QSettings *s)
{
    *this = CompletionSettings(); // Assign defaults

    s->beginGroup(settingsGroup);
    m_caseSensitivity = (CaseSensitivity)
            s->value(caseSensitivityKey, m_caseSensitivity).toInt();
    m_completionTrigger = (CompletionTrigger)
            s->value(completionTriggerKey, m_completionTrigger).toInt();
    m_automaticProposalTimeoutInMs =
            s->value(automaticProposalTimeoutKey, m_automaticProposalTimeoutInMs).toInt();
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
    s->endGroup();
}

bool CompletionSettings::equals(const CompletionSettings &cs) const
{
    return m_caseSensitivity                == cs.m_caseSensitivity
        && m_completionTrigger              == cs.m_completionTrigger
        && m_automaticProposalTimeoutInMs   == cs.m_automaticProposalTimeoutInMs
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
        ;
}
