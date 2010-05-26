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
#include <QtCore/QLatin1String>

#ifdef Q_OS_LINUX
#include <QtCore/QDir>
#include <QtCore/QProcess>
#endif

namespace TextEditor {
namespace Internal {

void applyDefaults(HighlighterSettings *settings)
{
    settings->m_definitionFilesPath.clear();

#ifdef Q_OS_LINUX
    static const QLatin1String kateSyntax("/share/apps/katepart/syntax");

    // Wild guess.
    QDir dir(QLatin1String("/usr") + kateSyntax);
    if (dir.exists()) {
        settings->m_definitionFilesPath = dir.path();
    } else {
        // Try kde-config.
        QProcess process;
        process.start(QLatin1String("kde-config"), QStringList(QLatin1String("--prefix")));
        if (process.waitForStarted(5000)) {
            process.waitForFinished(5000);
            QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
            output.remove(QLatin1Char('\n'));
            dir.setPath(output + kateSyntax);
            if (dir.exists())
                settings->m_definitionFilesPath = dir.path();
        }
    }
#endif

    if (settings->m_definitionFilesPath.isEmpty())
        settings->m_definitionFilesPath = Core::ICore::instance()->resourcePath() +
                                          QLatin1String("/generic-highlighter");
}

} // namespace Internal
} // namespace TextEditor

namespace {

static const QLatin1String kDefinitionFilesPath("DefinitionFilesPath");
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

HighlighterSettings::HighlighterSettings()
{}

void HighlighterSettings::toSettings(const QString &category, QSettings *s) const
{
    const QString &group = groupSpecifier(kGroupPostfix, category);
    s->beginGroup(group);
    s->setValue(kDefinitionFilesPath, m_definitionFilesPath);
    s->endGroup();
}

void HighlighterSettings::fromSettings(const QString &category, QSettings *s)
{
    const QString &group = groupSpecifier(kGroupPostfix, category);
    s->beginGroup(group);

    if (!s->contains(kDefinitionFilesPath))
        applyDefaults(this);
    else
        m_definitionFilesPath = s->value(kDefinitionFilesPath, QString()).toString();

    s->endGroup();
}

bool HighlighterSettings::equals(const HighlighterSettings &highlighterSettings) const
{
    return m_definitionFilesPath == highlighterSettings.m_definitionFilesPath;
}
