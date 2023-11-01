// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "highlightersettings.h"

#include "texteditorconstants.h"

#include <coreplugin/icore.h>

#include <utils/qtcsettings.h>

using namespace Utils;

namespace TextEditor {

const char kDefinitionFilesPath[] = "UserDefinitionFilesPath";
const char kIgnoredFilesPatterns[] = "IgnoredFilesPatterns";

static Key groupSpecifier(const Key &postFix, const Key &category)
{
    if (category.isEmpty())
        return postFix;
    return Key(category + postFix);
}

void HighlighterSettings::toSettings(const Key &category, QtcSettings *s) const
{
    const Key group = groupSpecifier(Constants::HIGHLIGHTER_SETTINGS_CATEGORY, category);
    s->beginGroup(group);
    s->setValue(kDefinitionFilesPath, m_definitionFilesPath.toSettings());
    s->setValue(kIgnoredFilesPatterns, ignoredFilesPatterns());
    s->endGroup();
}

void HighlighterSettings::fromSettings(const Key &category, QtcSettings *s)
{
    const Key group = groupSpecifier(Constants::HIGHLIGHTER_SETTINGS_CATEGORY, category);
    s->beginGroup(group);
    m_definitionFilesPath = FilePath::fromSettings(s->value(kDefinitionFilesPath));
    if (!s->contains(kDefinitionFilesPath))
        assignDefaultDefinitionsPath();
    else
        m_definitionFilesPath = FilePath::fromSettings(s->value(kDefinitionFilesPath));
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
