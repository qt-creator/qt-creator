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

#include "gnumakeparser.h"
#include "taskwindow.h"

#include <QtCore/QDir>
#include <QtCore/QFile>

using namespace ProjectExplorer;

GnuMakeParser::GnuMakeParser(const QString &dir)
{
    //make[4]: Entering directory `/some/dir'
    m_makeDir.setPattern("^(?:mingw32-)?make.*: (\\w+) directory .(.+).$");
    m_makeDir.setMinimal(true);
    addDirectory(dir);
}

void GnuMakeParser::stdOutput(const QString &line)
{
    QString lne = line.trimmed();

    if (m_makeDir.indexIn(lne) > -1) {
        if (m_makeDir.cap(1) == "Leaving")
            removeDirectory(m_makeDir.cap(2));
        else
            addDirectory(m_makeDir.cap(2));
        return;
    }

    IOutputParser::stdOutput(line);
}

void GnuMakeParser::addDirectory(const QString &dir)
{
    if (dir.isEmpty() || m_directories.contains(dir))
        return;
    m_directories.append(dir);
}

void GnuMakeParser::removeDirectory(const QString &dir)
{
    m_directories.removeAll(dir);
}

void GnuMakeParser::taskAdded(const ProjectExplorer::TaskWindow::Task &task)
{
    ProjectExplorer::TaskWindow::Task editable(task);
    QString filePath(QDir::cleanPath(task.file.trimmed()));

    if (!filePath.isEmpty() && !QDir::isAbsolutePath(filePath)) {
        QList<QFileInfo> possibleFiles;
        foreach (const QString &dir, m_directories) {
            QFileInfo candidate(dir + QLatin1Char('/') + filePath);
            if (candidate.exists()
                && !possibleFiles.contains(candidate)) {
                possibleFiles << candidate;
            }
        }
        if (possibleFiles.size() == 1)
            editable.file = possibleFiles.first().filePath();
        // Let the Makestep apply additional heuristics (based on
        // files in ther project) if we can not uniquely
        // identify the file!
    }

    IOutputParser::taskAdded(editable);
}
