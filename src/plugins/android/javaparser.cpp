// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "javaparser.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>
#include <QFileInfo>

using namespace Android::Internal;
using namespace ProjectExplorer;

JavaParser::JavaParser() :
  m_javaRegExp(QLatin1String("^(.*\\[javac\\]\\s)(.*\\.java):(\\d+):(.*)$"))
{ }

void JavaParser::setProjectFileList(const Utils::FilePaths &fileList)
{
    m_fileList = fileList;
}

void JavaParser::setBuildDirectory(const Utils::FilePath &buildDirectory)
{
    m_buildDirectory = buildDirectory;
}

void JavaParser::setSourceDirectory(const Utils::FilePath &sourceDirectory)
{
    m_sourceDirectory = sourceDirectory;
}

Utils::OutputLineParser::Result JavaParser::handleLine(const QString &line,
                                                       Utils::OutputFormat type)
{
    Q_UNUSED(type);
    const QRegularExpressionMatch match = m_javaRegExp.match(line);
    if (!match.hasMatch())
        return Status::NotHandled;

    bool ok;
    int lineno = match.captured(3).toInt(&ok);
    if (!ok)
        lineno = -1;
    Utils::FilePath file = Utils::FilePath::fromUserInput(match.captured(2));
    if (file.isChildOf(m_buildDirectory)) {
        Utils::FilePath relativePath = file.relativeChildPath(m_buildDirectory);
        file = m_sourceDirectory.pathAppended(relativePath.toString());
    }
    if (file.toFileInfo().isRelative()) {
        for (int i = 0; i < m_fileList.size(); i++)
            if (m_fileList[i].endsWith(file.toString())) {
                file = m_fileList[i];
                break;
            }
    }

    CompileTask task(Task::Error,
                     match.captured(4).trimmed(),
                     absoluteFilePath(file),
                     lineno);
    LinkSpecs linkSpecs;
    addLinkSpecForAbsoluteFilePath(linkSpecs, task.file, task.line, match, 2);
    scheduleTask(task, 1);
    return {Status::Done, linkSpecs};
}
