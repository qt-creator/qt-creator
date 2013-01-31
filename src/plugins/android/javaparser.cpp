/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "javaparser.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>

using namespace Android::Internal;
using namespace ProjectExplorer;

JavaParser::JavaParser() :
  m_javaRegExp(QLatin1String("^(.*\\[javac\\]\\s)(.*\\.java):(\\d+):(.*)$"))
{
}

void JavaParser::stdOutput(const QString &line)
{
    stdError(line);
}

void JavaParser::stdError(const QString &line)
{
    if (m_javaRegExp.indexIn(line) > -1) {
        bool ok;
        int lineno = m_javaRegExp.cap(3).toInt(&ok);
        if (!ok)
            lineno = -1;
        QString file = m_javaRegExp.cap(2);
        for (int i = 0; i < m_fileList.size(); i++)
            if (m_fileList[i].endsWith(file)) {
                file = m_fileList[i];
                break;
            }

        Task task(Task::Error,
                  m_javaRegExp.cap(4).trimmed(),
                  Utils::FileName::fromString(file) /* filename */,
                  lineno,
                  Constants::TASK_CATEGORY_COMPILE);
        emit addTask(task);
        return;
    }
    IOutputParser::stdError(line);
}

void JavaParser::setProjectFileList(const QStringList &fileList)
{
    m_fileList = fileList;
}
