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

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QSettings>
#include <QStringList>

using namespace Utils;

namespace TextEditor {
namespace Internal {

FilePath findFallbackDefinitionsLocation()
{
    if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost()) {
        static const QLatin1String kateSyntaxPaths[] = {
            QLatin1String("/share/apps/katepart/syntax"),
            QLatin1String("/share/kde4/apps/katepart/syntax")
        };

        // Some wild guesses.
        for (const auto &kateSyntaxPath : kateSyntaxPaths) {
            const FilePath paths[] = {
                FilePath("/usr") / kateSyntaxPath,
                FilePath("/usr/local") / kateSyntaxPath,
                FilePath("/opt") / kateSyntaxPath
            };
            for (const FilePath &path : paths) {
                if (path.exists() && !path.dirEntries({"*.xml"}).isEmpty())
                    return path;
            }
        }

        // Try kde-config.
        const FilePath programs[] = {"kde-config", "kde4-config"};
        for (const FilePath &program : programs) {
            QtcProcess process;
            process.setTimeoutS(5);
            process.setCommand({program, {"--prefix"}});
            process.runBlocking();
            if (process.result() == QtcProcess::FinishedWithSuccess) {
                QString output = process.stdOut();
                output.remove('\n');
                const FilePath dir = FilePath::fromString(output);
                for (auto &kateSyntaxPath : kateSyntaxPaths) {
                    const FilePath path = dir / kateSyntaxPath;
                    if (path.exists() && !path.dirEntries({"*.xml"}).isEmpty())
                        return path;
                }
            }
        }
    }

    const FilePath dir = Core::ICore::resourcePath("generic-highlighter");
    if (dir.exists() && !dir.dirEntries({"*.xml"}).isEmpty())
        return dir;

    return {};
}

} // namespace Internal

const QLatin1String kDefinitionFilesPath("UserDefinitionFilesPath");
const QLatin1String kIgnoredFilesPatterns("IgnoredFilesPatterns");

static QString groupSpecifier(const QString &postFix, const QString &category)
{
    if (category.isEmpty())
        return postFix;
    return QString(category + postFix);
}

void HighlighterSettings::toSettings(const QString &category, QSettings *s) const
{
    const QString &group = groupSpecifier(Constants::HIGHLIGHTER_SETTINGS_CATEGORY, category);
    s->beginGroup(group);
    s->setValue(kDefinitionFilesPath, m_definitionFilesPath.toVariant());
    s->setValue(kIgnoredFilesPatterns, ignoredFilesPatterns());
    s->endGroup();
}

void HighlighterSettings::fromSettings(const QString &category, QSettings *s)
{
    const QString &group = groupSpecifier(Constants::HIGHLIGHTER_SETTINGS_CATEGORY, category);
    s->beginGroup(group);
    m_definitionFilesPath = FilePath::fromVariant(s->value(kDefinitionFilesPath));
    if (!s->contains(kDefinitionFilesPath))
        assignDefaultDefinitionsPath();
    else
        m_definitionFilesPath = FilePath::fromVariant(s->value(kDefinitionFilesPath));
    if (!s->contains(kIgnoredFilesPatterns))
        assignDefaultIgnoredPatterns();
    else
        setIgnoredFilesPatterns(s->value(kIgnoredFilesPatterns, QString()).toString());
    s->endGroup();
}

void HighlighterSettings::setIgnoredFilesPatterns(const QString &patterns)
{
    setExpressionsFromList(patterns.split(',', Qt::SkipEmptyParts));
}

QString HighlighterSettings::ignoredFilesPatterns() const
{
    return listFromExpressions().join(',');
}

void HighlighterSettings::assignDefaultIgnoredPatterns()
{
    setExpressionsFromList({"*.txt",
                            "LICENSE*",
                            "README",
                            "INSTALL",
                            "COPYING",
                            "NEWS",
                            "qmldir"});
}

void HighlighterSettings::assignDefaultDefinitionsPath()
{
    const FilePath path = Core::ICore::userResourcePath("generic-highlighter");
    if (path.exists() || path.ensureWritableDir())
        m_definitionFilesPath = path;
}

bool HighlighterSettings::isIgnoredFilePattern(const QString &fileName) const
{
    for (const QRegularExpression &regExp : m_ignoredFiles)
        if (fileName.indexOf(regExp) != -1)
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
    QRegularExpression regExp;
    regExp.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    for (const QString &pattern : patterns) {
        regExp.setPattern(QRegularExpression::wildcardToRegularExpression(pattern));
        m_ignoredFiles.append(regExp);
    }
}

QStringList HighlighterSettings::listFromExpressions() const
{
    QStringList patterns;
    for (const QRegularExpression &regExp : m_ignoredFiles)
        patterns.append(regExp.pattern());
    return patterns;
}

} // TextEditor
