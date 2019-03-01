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

#include "highlightersettings.h"

#include "texteditorconstants.h"

#include <coreplugin/icore.h>
#include <utils/hostosinfo.h>
#include <utils/synchronousprocess.h>

#include <QSettings>
#include <QLatin1String>
#include <QLatin1Char>
#include <QDir>
#include <QFile>
#include <QStringList>

namespace TextEditor {
namespace Internal {

QString findFallbackDefinitionsLocation()
{
    QDir dir;
    dir.setNameFilters(QStringList(QLatin1String("*.xml")));

    if (Utils::HostOsInfo::isAnyUnixHost() && !Utils::HostOsInfo::isMacHost()) {
        static const QLatin1String kateSyntaxPaths[] = {
            QLatin1String("/share/apps/katepart/syntax"),
            QLatin1String("/share/kde4/apps/katepart/syntax")
        };

        // Some wild guesses.
        for (const auto &kateSyntaxPath : kateSyntaxPaths) {
            const QStringList paths = {
                QLatin1String("/usr") + kateSyntaxPath,
                QLatin1String("/usr/local") + kateSyntaxPath,
                QLatin1String("/opt") + kateSyntaxPath
            };
            for (const auto &path : paths) {
                dir.setPath(path);
                if (dir.exists() && !dir.entryInfoList().isEmpty())
                    return dir.path();
            }
        }

        // Try kde-config.
        const QStringList programs = {QLatin1String("kde-config"), QLatin1String("kde4-config")};
        for (auto &program : programs) {
            Utils::SynchronousProcess process;
            process.setTimeoutS(5);
            Utils::SynchronousProcessResponse response
                    = process.runBlocking(program, QStringList(QLatin1String("--prefix")));
            if (response.result == Utils::SynchronousProcessResponse::Finished) {
                QString output = response.stdOut();
                output.remove(QLatin1Char('\n'));
                for (auto &kateSyntaxPath : kateSyntaxPaths) {
                    dir.setPath(output + kateSyntaxPath);
                    if (dir.exists() && !dir.entryInfoList().isEmpty())
                        return dir.path();
                }
            }
        }
    }

    dir.setPath(Core::ICore::resourcePath() + QLatin1String("/generic-highlighter"));
    if (dir.exists() && !dir.entryInfoList().isEmpty())
        return dir.path();

    return QString();
}

} // namespace Internal
} // namespace TextEditor

namespace {

static const QLatin1String kDefinitionFilesPath("UserDefinitionFilesPath");
static const QLatin1String kIgnoredFilesPatterns("IgnoredFilesPatterns");

QString groupSpecifier(const QString &postFix, const QString &category)
{
    if (category.isEmpty())
        return postFix;
    return QString(category + postFix);
}

} // namespace anonymous

using namespace TextEditor;
using namespace Internal;

void HighlighterSettings::toSettings(const QString &category, QSettings *s) const
{
    const QString &group = groupSpecifier(Constants::HIGHLIGHTER_SETTINGS_CATEGORY, category);
    s->beginGroup(group);
    s->setValue(kDefinitionFilesPath, m_definitionFilesPath);
    s->setValue(kIgnoredFilesPatterns, ignoredFilesPatterns());
    s->endGroup();
}

void HighlighterSettings::fromSettings(const QString &category, QSettings *s)
{
    const QString &group = groupSpecifier(Constants::HIGHLIGHTER_SETTINGS_CATEGORY, category);
    s->beginGroup(group);
    m_definitionFilesPath = s->value(kDefinitionFilesPath, QString()).toString();
    if (!s->contains(kDefinitionFilesPath))
        assignDefaultDefinitionsPath();
    else
        m_definitionFilesPath = s->value(kDefinitionFilesPath).toString();
    if (!s->contains(kIgnoredFilesPatterns))
        assignDefaultIgnoredPatterns();
    else
        setIgnoredFilesPatterns(s->value(kIgnoredFilesPatterns, QString()).toString());
    s->endGroup();
}

void HighlighterSettings::setIgnoredFilesPatterns(const QString &patterns)
{
    setExpressionsFromList(patterns.split(QLatin1Char(','), QString::SkipEmptyParts));
}

QString HighlighterSettings::ignoredFilesPatterns() const
{
    return listFromExpressions().join(QLatin1Char(','));
}

void HighlighterSettings::assignDefaultIgnoredPatterns()
{
    QStringList patterns;
    patterns << QLatin1String("*.txt")
        << QLatin1String("LICENSE*")
        << QLatin1String("README")
        << QLatin1String("INSTALL")
        << QLatin1String("COPYING")
        << QLatin1String("NEWS")
        << QLatin1String("qmldir");
    setExpressionsFromList(patterns);
}

void HighlighterSettings::assignDefaultDefinitionsPath()
{
    const QString &path =
        Core::ICore::userResourcePath() + QLatin1String("/generic-highlighter");
    if (QFile::exists(path) || QDir().mkpath(path))
        m_definitionFilesPath = path;
}

bool HighlighterSettings::isIgnoredFilePattern(const QString &fileName) const
{
    for (const QRegExp &regExp : m_ignoredFiles)
        if (regExp.indexIn(fileName) != -1)
            return true;

    return false;
}

bool HighlighterSettings::equals(const HighlighterSettings &highlighterSettings) const
{
    return m_definitionFilesPath == highlighterSettings.m_definitionFilesPath &&
           m_ignoredFiles == highlighterSettings.m_ignoredFiles;
}

void HighlighterSettings::setExpressionsFromList(const QStringList &patterns)
{
    m_ignoredFiles.clear();
    QRegExp regExp;
    regExp.setCaseSensitivity(Qt::CaseInsensitive);
    regExp.setPatternSyntax(QRegExp::Wildcard);
    for (const QString &pattern : patterns) {
        regExp.setPattern(pattern);
        m_ignoredFiles.append(regExp);
    }
}

QStringList HighlighterSettings::listFromExpressions() const
{
    QStringList patterns;
    for (const QRegExp &regExp : m_ignoredFiles)
        patterns.append(regExp.pattern());
    return patterns;
}
