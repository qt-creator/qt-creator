/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "completionsettings.h"

#include <QSettings>

static const char groupPostfix[] = "Completion";
static const char caseSensitivityKey[] = "CaseSensitivity";
static const char completionTriggerKey[] = "CompletionTrigger";
static const char automaticProposalTimeoutKey[] = "AutomaticProposalTimeout";
static const char autoInsertBracesKey[] = "AutoInsertBraces";
static const char surroundingAutoBracketsKey[] = "SurroundingAutoBrackets";
static const char partiallyCompleteKey[] = "PartiallyComplete";
static const char spaceAfterFunctionNameKey[] = "SpaceAfterFunctionName";
static const char autoSplitStringsKey[] = "AutoSplitStrings";

using namespace TextEditor;

CompletionSettings::CompletionSettings()
    : m_caseSensitivity(CaseInsensitive)
    , m_completionTrigger(AutomaticCompletion)
    , m_automaticProposalTimeoutInMs(400)
    , m_autoInsertBrackets(true)
    , m_surroundingAutoBrackets(true)
    , m_partiallyComplete(true)
    , m_spaceAfterFunctionName(false)
    , m_autoSplitStrings(true)
{
}

void CompletionSettings::toSettings(const QString &category, QSettings *s) const
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);

    s->beginGroup(group);
    s->setValue(QLatin1String(caseSensitivityKey), (int) m_caseSensitivity);
    s->setValue(QLatin1String(completionTriggerKey), (int) m_completionTrigger);
    s->setValue(QLatin1String(automaticProposalTimeoutKey), m_automaticProposalTimeoutInMs);
    s->setValue(QLatin1String(autoInsertBracesKey), m_autoInsertBrackets);
    s->setValue(QLatin1String(surroundingAutoBracketsKey), m_surroundingAutoBrackets);
    s->setValue(QLatin1String(partiallyCompleteKey), m_partiallyComplete);
    s->setValue(QLatin1String(spaceAfterFunctionNameKey), m_spaceAfterFunctionName);
    s->setValue(QLatin1String(autoSplitStringsKey), m_autoSplitStrings);
    s->endGroup();
}

void CompletionSettings::fromSettings(const QString &category, const QSettings *s)
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);
    group += QLatin1Char('/');

    *this = CompletionSettings(); // Assign defaults

    m_caseSensitivity = (CaseSensitivity) s->value(group + QLatin1String(caseSensitivityKey), m_caseSensitivity).toInt();
    m_completionTrigger = (CompletionTrigger) s->value(group + QLatin1String(completionTriggerKey), m_completionTrigger).toInt();
    m_automaticProposalTimeoutInMs = s->value(group + QLatin1String(automaticProposalTimeoutKey), m_automaticProposalTimeoutInMs).toInt();
    m_autoInsertBrackets = s->value(group + QLatin1String(autoInsertBracesKey), m_autoInsertBrackets).toBool();
    m_surroundingAutoBrackets = s->value(group + QLatin1String(surroundingAutoBracketsKey), m_surroundingAutoBrackets).toBool();
    m_partiallyComplete = s->value(group + QLatin1String(partiallyCompleteKey), m_partiallyComplete).toBool();
    m_spaceAfterFunctionName = s->value(group + QLatin1String(spaceAfterFunctionNameKey), m_spaceAfterFunctionName).toBool();
    m_autoSplitStrings = s->value(group + QLatin1String(autoSplitStringsKey), m_autoSplitStrings).toBool();
}

bool CompletionSettings::equals(const CompletionSettings &cs) const
{
    return m_caseSensitivity == cs.m_caseSensitivity
        && m_completionTrigger == cs.m_completionTrigger
        && m_automaticProposalTimeoutInMs == cs.m_automaticProposalTimeoutInMs
        && m_autoInsertBrackets == cs.m_autoInsertBrackets
        && m_surroundingAutoBrackets == cs.m_surroundingAutoBrackets
        && m_partiallyComplete == cs.m_partiallyComplete
        && m_spaceAfterFunctionName == cs.m_spaceAfterFunctionName
        && m_autoSplitStrings == cs.m_autoSplitStrings
        ;
}
