/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "completionsettings.h"

#include <QtCore/QSettings>

static const char * const groupPostfix = "Completion";
static const char * const caseSensitivityKey = "CaseSensitivity";
static const char * const autoInsertBracesKey = "AutoInsertBraces";
static const char * const partiallyCompleteKey = "PartiallyComplete";
static const char * const spaceAfterFunctionNameKey = "SpaceAfterFunctionName";

using namespace TextEditor;

CompletionSettings::CompletionSettings()
    : m_caseSensitivity(CaseInsensitive)
    , m_autoInsertBrackets(true)
    , m_partiallyComplete(true)
    , m_spaceAfterFunctionName(false)
{
}

void CompletionSettings::toSettings(const QString &category, QSettings *s) const
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);

    s->beginGroup(group);
    s->setValue(QLatin1String(caseSensitivityKey), (int) m_caseSensitivity);
    s->setValue(QLatin1String(autoInsertBracesKey), m_autoInsertBrackets);
    s->setValue(QLatin1String(partiallyCompleteKey), m_partiallyComplete);
    s->setValue(QLatin1String(spaceAfterFunctionNameKey), m_spaceAfterFunctionName);
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
    m_autoInsertBrackets = s->value(group + QLatin1String(autoInsertBracesKey), m_autoInsertBrackets).toBool();
    m_partiallyComplete = s->value(group + QLatin1String(partiallyCompleteKey), m_partiallyComplete).toBool();
    m_spaceAfterFunctionName = s->value(group + QLatin1String(spaceAfterFunctionNameKey), m_spaceAfterFunctionName).toBool();
}

bool CompletionSettings::equals(const CompletionSettings &cs) const
{
    return m_caseSensitivity == cs.m_caseSensitivity
        && m_autoInsertBrackets == cs.m_autoInsertBrackets
        && m_partiallyComplete == cs.m_partiallyComplete
        && m_spaceAfterFunctionName == cs.m_spaceAfterFunctionName
        ;
}
