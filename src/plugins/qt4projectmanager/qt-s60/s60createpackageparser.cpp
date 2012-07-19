/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "s60createpackageparser.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>

using namespace Qt4ProjectManager::Internal;

S60CreatePackageParser::S60CreatePackageParser(const QString &packageName) :
    m_packageName(packageName),
    m_needPassphrase(false)
{
    setObjectName(QLatin1String("S60CreatePackageParser"));
    m_signSis.setPattern(QLatin1String("^(\\s*|\\(\\d+\\)\\s*:\\s*)(error\\s?:\\s?)+(.+)$"));
    m_signSis.setMinimal(true);
    m_signSis.setCaseSensitivity(Qt::CaseInsensitive);
}

bool S60CreatePackageParser::parseLine(const QString &line)
{
    if (line.startsWith(QLatin1String("Patching: "))) {
        m_patchingLines.append(line.mid(10).trimmed());
        return true;
    }
    if (!m_patchingLines.isEmpty()) {
        emit packageWasPatched(m_packageName, m_patchingLines);

        QString lines = m_patchingLines.join(QLatin1String("\n"));
        m_patchingLines.clear();
        //: %1 package name, %2 will be replaced by a list of patching lines.
        QString message = tr("The binary package '%1' was patched to be installable after being self-signed.\n%2\n"
                             "Use a developer certificate or any other signing option to prevent "
                             "this patching from happening.").
                arg(m_packageName, lines);
        ProjectExplorer::Task task(ProjectExplorer::Task::Warning, message, Utils::FileName(), -1,
                                   Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));

        QTextLayout::FormatRange fr;
        fr.start = message.indexOf(lines);
        fr.length = lines.length();
        fr.format.setFontItalic(true);
        task.formats.append(fr);

        emit addTask(task);
    }

    if (m_signSis.indexIn(line) > -1) {
        QString errorMessage(m_signSis.cap(3));
        if (errorMessage.contains(QLatin1String("bad password"))
            || errorMessage.contains(QLatin1String("bad decrypt")))
            m_needPassphrase = true;
        else if (errorMessage.contains(QLatin1String("Cannot open file"))
                 && errorMessage.contains(QLatin1String("smartinstaller")))
            emit addTask(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                               tr("Cannot create Smart Installer package "
                                                  "as the Smart Installer's base file is missing. "
                                                  "Please ensure that it is located in the SDK."),
                                               Utils::FileName(), -1,
                                               Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
        else
            emit addTask(ProjectExplorer::Task(ProjectExplorer::Task::Error, errorMessage, Utils::FileName(), -1,
                                               Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
        return true;
    }
    return false;
}

void S60CreatePackageParser::stdOutput(const QString &line)
{
    if (!parseLine(line))
        IOutputParser::stdOutput(line);
}

void S60CreatePackageParser::stdError(const QString &line)
{
    if (!parseLine(line))
        IOutputParser::stdError(line);
}

bool S60CreatePackageParser::needPassphrase() const
{
    return m_needPassphrase;
}

/* STDOUT:
make[1]: Entering directory `C:/temp/test/untitled131'
createpackage.bat -g  untitled131_template.pkg RELEASE-armv5
Auto-patching capabilities for self signed package.

Patching package file and relevant binaries...
Patching: Removed dependency to qt.sis (0x2001E61C) to avoid installation issues in case qt.sis is also patched.


NOTE: A patched package may not work as expected due to reduced capabilities and other modifications,
      so it should not be used for any kind of Symbian signing or distribution!
      Use a proper certificate to avoid the need to patch the package.

Processing untitled131_release-armv5.pkg...


and errors like:
(35) : error: Cannot find file : c:/QtSDK/Symbian/SDKs/Symbian3Qt471/epoc32/data/z/resource/apps/untitledSymbian.mif
*/
