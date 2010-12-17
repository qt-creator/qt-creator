/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "settings.h"

#include <QtCore/QSettings>

static const char *syntaxKey = "Syntax";
static const char *minimalKey = "Minimal";
static const char *caseSensitiveKey = "CaseSensitive";
static const char *patternKey = "Patterns";
static const char *currentPatternKey = "CurrentPattern";
static const char *matchKey = "Matches";
static const char *currentMatchKey = "CurrentMatch";
static const char *patternDefault = "[A-Za-z_]+([A-Za-z_0-9]*)";
static const char *matchDefault = "(10 + delta4) * 32";
static const char *settingsGroup = "RegExp";

namespace RegExp {
namespace Internal {

Settings::Settings() :
    m_patternSyntax(QRegExp::RegExp),
    m_minimal(false),
    m_caseSensitive(true),
    m_patterns(QLatin1String(patternDefault)),
    m_currentPattern(m_patterns.front()),
    m_matches(QLatin1String(matchDefault)),
    m_currentMatch(m_matches.front())
{
}

void Settings::clear()
{
    *this = Settings();
}

void Settings::fromQSettings(QSettings *s)
{
    clear();
    s->beginGroup(QLatin1String(settingsGroup));
    m_patternSyntax = static_cast<QRegExp::PatternSyntax>(s->value(QLatin1String(syntaxKey), m_patternSyntax).toInt());
    m_minimal = s->value(QLatin1String(minimalKey), m_minimal).toBool();
    m_caseSensitive = s->value(QLatin1String(caseSensitiveKey), m_caseSensitive).toBool();
    m_patterns = s->value(QLatin1String(patternKey), m_patterns).toStringList();
    m_currentPattern = s->value(QLatin1String(currentPatternKey), m_currentPattern).toString();
    m_matches = s->value(QLatin1String(matchKey), m_matches).toStringList();
    m_currentMatch = s->value(QLatin1String(currentMatchKey), m_currentMatch).toString();
    s->endGroup();
}

void Settings::toQSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(settingsGroup));
    s->setValue(QLatin1String(syntaxKey), m_patternSyntax);
    s->setValue(QLatin1String(minimalKey), m_minimal);
    s->setValue(QLatin1String(caseSensitiveKey), m_caseSensitive);
    s->setValue(QLatin1String(patternKey), m_patterns);
    s->setValue(QLatin1String(currentPatternKey), m_currentPattern);
    s->setValue(QLatin1String(matchKey), m_matches);
    s->setValue(QLatin1String(currentMatchKey), m_currentMatch);
    s->endGroup();
}

} // namespace Internal
} // namespace Settings;
