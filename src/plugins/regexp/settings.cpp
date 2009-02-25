/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
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
