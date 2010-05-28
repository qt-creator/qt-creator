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

#include "highlightersettings.h"

#include <coreplugin/icore.h>

#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QLatin1String>

#ifdef Q_OS_UNIX
#include <QtCore/QDir>
#include <QtCore/QProcess>
#endif

namespace TextEditor {
namespace Internal {

QString findDefinitionsLocation()
{
    QString definitionsLocation;

#ifdef Q_OS_UNIX
    static const QLatin1String kateSyntax("/share/apps/katepart/syntax");

    // Some wild guesses.
    QDir dir;
    QStringList paths;
    paths << QLatin1String("/usr") + kateSyntax
          << QLatin1String("/usr/local") + kateSyntax
          << QLatin1String("/opt") + kateSyntax;
    foreach (const QString &path, paths) {
        dir.setPath(path);
        if (dir.exists()) {
            definitionsLocation = path;
            break;
        }
    }

    if (definitionsLocation.isEmpty()) {
        // Try kde-config.
        QProcess process;
        process.start(QLatin1String("kde-config"), QStringList(QLatin1String("--prefix")));
        if (process.waitForStarted(5000)) {
            process.waitForFinished(5000);
            QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
            output.remove(QLatin1Char('\n'));
            dir.setPath(output + kateSyntax);
            if (dir.exists())
                definitionsLocation = dir.path();
        }
    }
#endif

    if (definitionsLocation.isEmpty())
        definitionsLocation = Core::ICore::instance()->resourcePath() +
                              QLatin1String("/generic-highlighter");

    return definitionsLocation;
}

} // namespace Internal
} // namespace TextEditor

namespace {

static const QLatin1String kDefinitionFilesPath("DefinitionFilesPath");
static const QLatin1String kAlertWhenDefinitionIsNotFound("AlertWhenDefinitionsIsNotFound");
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

HighlighterSettings::HighlighterSettings() : m_alertWhenNoDefinition(true)
{}

void HighlighterSettings::toSettings(const QString &category, QSettings *s) const
{
    const QString &group = groupSpecifier(kGroupPostfix, category);
    s->beginGroup(group);
    s->setValue(kDefinitionFilesPath, m_definitionFilesPath);
    s->setValue(kAlertWhenDefinitionIsNotFound, m_alertWhenNoDefinition);
    s->endGroup();
}

void HighlighterSettings::fromSettings(const QString &category, QSettings *s)
{
    const QString &group = groupSpecifier(kGroupPostfix, category);
    s->beginGroup(group);
    if (!s->contains(kDefinitionFilesPath))
        m_definitionFilesPath = findDefinitionsLocation();
    else
        m_definitionFilesPath = s->value(kDefinitionFilesPath, QString()).toString();
    m_alertWhenNoDefinition = s->value(kAlertWhenDefinitionIsNotFound, true).toBool();
    s->endGroup();
}

bool HighlighterSettings::equals(const HighlighterSettings &highlighterSettings) const
{
    return m_definitionFilesPath == highlighterSettings.m_definitionFilesPath &&
           m_alertWhenNoDefinition == highlighterSettings.m_alertWhenNoDefinition;
}
