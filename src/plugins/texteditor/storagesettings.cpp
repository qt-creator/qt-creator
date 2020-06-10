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

#include "storagesettings.h"

#include <utils/hostosinfo.h>
#include <utils/settingsutils.h>

#include <QRegularExpression>
#include <QSettings>
#include <QString>

namespace TextEditor {

static const char cleanWhitespaceKey[] = "cleanWhitespace";
static const char inEntireDocumentKey[] = "inEntireDocument";
static const char addFinalNewLineKey[] = "addFinalNewLine";
static const char cleanIndentationKey[] = "cleanIndentation";
static const char skipTrailingWhitespaceKey[] = "skipTrailingWhitespace";
static const char ignoreFileTypesKey[] = "ignoreFileTypes";
static const char groupPostfix[] = "StorageSettings";
static const char defaultTrailingWhitespaceBlacklist[] = "*.md, *.MD, Makefile";

StorageSettings::StorageSettings()
    : m_ignoreFileTypes(defaultTrailingWhitespaceBlacklist),
      m_cleanWhitespace(true),
      m_inEntireDocument(false),
      m_addFinalNewLine(true),
      m_cleanIndentation(true),
      m_skipTrailingWhitespace(true)
{
}

void StorageSettings::toSettings(const QString &category, QSettings *s) const
{
    Utils::toSettings(QLatin1String(groupPostfix), category, s, this);
}

void StorageSettings::fromSettings(const QString &category, const QSettings *s)
{
    *this = StorageSettings();
    Utils::fromSettings(QLatin1String(groupPostfix), category, s, this);
}

void StorageSettings::toMap(const QString &prefix, QVariantMap *map) const
{
    map->insert(prefix + QLatin1String(cleanWhitespaceKey), m_cleanWhitespace);
    map->insert(prefix + QLatin1String(inEntireDocumentKey), m_inEntireDocument);
    map->insert(prefix + QLatin1String(addFinalNewLineKey), m_addFinalNewLine);
    map->insert(prefix + QLatin1String(cleanIndentationKey), m_cleanIndentation);
    map->insert(prefix + QLatin1String(skipTrailingWhitespaceKey), m_skipTrailingWhitespace);
    map->insert(prefix + QLatin1String(ignoreFileTypesKey), m_ignoreFileTypes.toLatin1().data());
}

void StorageSettings::fromMap(const QString &prefix, const QVariantMap &map)
{
    m_cleanWhitespace =
        map.value(prefix + QLatin1String(cleanWhitespaceKey), m_cleanWhitespace).toBool();
    m_inEntireDocument =
        map.value(prefix + QLatin1String(inEntireDocumentKey), m_inEntireDocument).toBool();
    m_addFinalNewLine =
        map.value(prefix + QLatin1String(addFinalNewLineKey), m_addFinalNewLine).toBool();
    m_cleanIndentation =
        map.value(prefix + QLatin1String(cleanIndentationKey), m_cleanIndentation).toBool();
    m_skipTrailingWhitespace =
        map.value(prefix + QLatin1String(skipTrailingWhitespaceKey), m_skipTrailingWhitespace).toBool();
    m_ignoreFileTypes =
        map.value(prefix + QLatin1String(ignoreFileTypesKey), m_ignoreFileTypes).toString();
}

bool StorageSettings::removeTrailingWhitespace(const QString &fileName) const
{
    // if the user has elected not to trim trailing whitespace altogether, then
    // early out here
    if (!m_skipTrailingWhitespace) {
        return true;
    }

    const QString ignoreFileTypesRegExp(R"(\s*((?>\*\.)?[\w\d\.\*]+)[,;]?\s*)");

    // use the ignore-files regex to extract the specified file patterns
    QRegularExpression re(ignoreFileTypesRegExp);
    QRegularExpressionMatchIterator iter = re.globalMatch(m_ignoreFileTypes);

    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString pattern = match.captured(1);

        QRegularExpression patternRegExp(QRegularExpression::wildcardToRegularExpression(pattern));
        QRegularExpressionMatch patternMatch = patternRegExp.match(fileName);
        if (patternMatch.hasMatch()) {
            // if the filename has a pattern we want to ignore, then we need to return
            // false ("don't remove trailing whitespace")
            return false;
        }
    }

    // the supplied pattern does not match, so we want to remove trailing whitespace
    return true;
}

bool StorageSettings::equals(const StorageSettings &ts) const
{
    return m_addFinalNewLine == ts.m_addFinalNewLine
        && m_cleanWhitespace == ts.m_cleanWhitespace
        && m_inEntireDocument == ts.m_inEntireDocument
        && m_cleanIndentation == ts.m_cleanIndentation
        && m_skipTrailingWhitespace == ts.m_skipTrailingWhitespace
        && m_ignoreFileTypes == ts.m_ignoreFileTypes;
}

} // namespace TextEditor
