/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "javaparser.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>
#include <QFileInfo>

using namespace Android::Internal;
using namespace ProjectExplorer;

JavaParser::JavaParser() :
  m_javaRegExp(QLatin1String("^(.*\\[javac\\]\\s)(.*\\.java):(\\d+):(.*)$"))
{ }

void JavaParser::stdOutput(const QString &line)
{
    parse(line);
    IOutputParser::stdOutput(line);
}

void JavaParser::stdError(const QString &line)
{
    parse(line);
    IOutputParser::stdError(line);
}

void JavaParser::setProjectFileList(const QStringList &fileList)
{
    m_fileList = fileList;
}

void JavaParser::setBuildDirectory(const Utils::FileName &buildDirectory)
{
    m_buildDirectory = buildDirectory;
}

void JavaParser::setSourceDirectory(const Utils::FileName &sourceDirectory)
{
    m_sourceDirectory = sourceDirectory;
}

void JavaParser::parse(const QString &line)
{
    if (m_javaRegExp.indexIn(line) > -1) {
        bool ok;
        int lineno = m_javaRegExp.cap(3).toInt(&ok);
        if (!ok)
            lineno = -1;
        Utils::FileName file = Utils::FileName::fromUserInput(m_javaRegExp.cap(2));
        if (file.isChildOf(m_buildDirectory)) {
            Utils::FileName relativePath = file.relativeChildPath(m_buildDirectory);
            file = m_sourceDirectory;
            file.appendPath(relativePath.toString());
        }

        if (file.toFileInfo().isRelative()) {
            for (int i = 0; i < m_fileList.size(); i++)
                if (m_fileList[i].endsWith(file.toString())) {
                    file = Utils::FileName::fromString(m_fileList[i]);
                    break;
                }
        }

        Task task(Task::Error,
                  m_javaRegExp.cap(4).trimmed(),
                  file /* filename */,
                  lineno,
                  Constants::TASK_CATEGORY_COMPILE);
        emit addTask(task, 1);
        return;
    }

}
