/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "highlightersettings.h"

#include <coreplugin/icore.h>

#include <QSettings>
#include <QLatin1String>
#include <QLatin1Char>
#include <QDir>
#include <QFile>
#include <QStringList>
#ifdef Q_OS_UNIX
#include <QProcess>
#endif

namespace TextEditor {
namespace Internal {

QString findFallbackDefinitionsLocation()
{
    QDir dir;
    dir.setNameFilters(QStringList(QLatin1String("*.xml")));

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    static const QLatin1String kateSyntax[] = {
        QLatin1String("/share/apps/katepart/syntax"),
        QLatin1String("/share/kde4/apps/katepart/syntax")
    };
    static const int kateSyntaxCount =
        sizeof(kateSyntax) / sizeof(kateSyntax[0]);

    // Some wild guesses.
    for (int i = 0; i < kateSyntaxCount; ++i) {
        QStringList paths;
        paths << QLatin1String("/usr") + kateSyntax[i]
              << QLatin1String("/usr/local") + kateSyntax[i]
              << QLatin1String("/opt") + kateSyntax[i];
        foreach (const QString &path, paths) {
            dir.setPath(path);
            if (dir.exists() && !dir.entryInfoList().isEmpty())
                return dir.path();
        }
    }

    // Try kde-config.
    QStringList programs;
    programs << QLatin1String("kde-config") << QLatin1String("kde4-config");
    foreach (const QString &program, programs) {
        QProcess process;
        process.start(program, QStringList(QLatin1String("--prefix")));
        if (process.waitForStarted(5000)) {
            process.waitForFinished(5000);
            QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
            output.remove(QLatin1Char('\n'));
            for (int i = 0; i < kateSyntaxCount; ++i) {
                dir.setPath(output + kateSyntax[i]);
                if (dir.exists() && !dir.entryInfoList().isEmpty())
                    return dir.path();
            }
        }
    }
#endif

    dir.setPath(Core::ICore::resourcePath() + QLatin1String("/generic-highlighter"));
    if (dir.exists() && !dir.entryInfoList().isEmpty())
        return dir.path();

    return QString();
}

} // namespace Internal
} // namespace TextEditor

namespace {

static const QLatin1String kDefinitionFilesPath("UserDefinitionFilesPath");
static const QLatin1String kFallbackDefinitionFilesPath("FallbackDefinitionFilesPath");
static const QLatin1String kAlertWhenDefinitionIsNotFound("AlertWhenDefinitionsIsNotFound");
static const QLatin1String kUseFallbackLocation("UseFallbackLocation");
static const QLatin1String kIgnoredFilesPatterns("IgnoredFilesPatterns");
static const QLatin1String kGroupPostfix("HighlighterSettings");

QString groupSpecifier(const QString &postFix, const QString &category)
{
    if (category.isEmpty())
        return postFix;
    return QString(category + postFix);
}

} // namespace anonymous

using namespace TextEditor;
using namespace Internal;

HighlighterSettings::HighlighterSettings() :
    m_alertWhenNoDefinition(true), m_useFallbackLocation(true)
{}

void HighlighterSettings::toSettings(const QString &category, QSettings *s) const
{
    const QString &group = groupSpecifier(kGroupPostfix, category);
    s->beginGroup(group);
    s->setValue(kDefinitionFilesPath, m_definitionFilesPath);
    s->setValue(kFallbackDefinitionFilesPath, m_fallbackDefinitionFilesPath);
    s->setValue(kAlertWhenDefinitionIsNotFound, m_alertWhenNoDefinition);
    s->setValue(kUseFallbackLocation, m_useFallbackLocation);
    s->setValue(kIgnoredFilesPatterns, ignoredFilesPatterns());
    s->endGroup();
}

void HighlighterSettings::fromSettings(const QString &category, QSettings *s)
{
    const QString &group = groupSpecifier(kGroupPostfix, category);
    s->beginGroup(group);
    m_definitionFilesPath = s->value(kDefinitionFilesPath, QString()).toString();
    if (!s->contains(kDefinitionFilesPath))
        assignDefaultDefinitionsPath();
    else
        m_definitionFilesPath = s->value(kDefinitionFilesPath).toString();
    if (!s->contains(kFallbackDefinitionFilesPath)) {
        m_fallbackDefinitionFilesPath = findFallbackDefinitionsLocation();
        if (m_fallbackDefinitionFilesPath.isEmpty())
            m_useFallbackLocation = false;
        else
            m_useFallbackLocation = true;
    } else {
        m_fallbackDefinitionFilesPath = s->value(kFallbackDefinitionFilesPath).toString();
        m_useFallbackLocation = s->value(kUseFallbackLocation, true).toBool();
    }
    m_alertWhenNoDefinition = s->value(kAlertWhenDefinitionIsNotFound, true).toBool();
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
    return listFromExpressions().join(QLatin1String(","));
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
    foreach (QRegExp regExp, m_ignoredFiles)
        if (regExp.indexIn(fileName) != -1)
            return true;

    return false;
}

bool HighlighterSettings::equals(const HighlighterSettings &highlighterSettings) const
{
    return m_definitionFilesPath == highlighterSettings.m_definitionFilesPath &&
           m_fallbackDefinitionFilesPath == highlighterSettings.m_fallbackDefinitionFilesPath &&
           m_alertWhenNoDefinition == highlighterSettings.m_alertWhenNoDefinition &&
           m_useFallbackLocation == highlighterSettings.m_useFallbackLocation &&
           m_ignoredFiles == highlighterSettings.m_ignoredFiles;
}

void HighlighterSettings::setExpressionsFromList(const QStringList &patterns)
{
    m_ignoredFiles.clear();
    QRegExp regExp;
    regExp.setCaseSensitivity(Qt::CaseInsensitive);
    regExp.setPatternSyntax(QRegExp::Wildcard);
    foreach (const QString &s, patterns) {
        regExp.setPattern(s);
        m_ignoredFiles.append(regExp);
    }
}

QStringList HighlighterSettings::listFromExpressions() const
{
    QStringList patterns;
    foreach (const QRegExp &regExp, m_ignoredFiles)
        patterns.append(regExp.pattern());
    return patterns;
}
