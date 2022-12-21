// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javaparser.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>

#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace Utils;

namespace Android::Internal {

JavaParser::JavaParser()
{ }

void JavaParser::setProjectFileList(const FilePaths &fileList)
{
    m_fileList = fileList;
}

void JavaParser::setBuildDirectory(const FilePath &buildDirectory)
{
    m_buildDirectory = buildDirectory;
}

void JavaParser::setSourceDirectory(const FilePath &sourceDirectory)
{
    m_sourceDirectory = sourceDirectory;
}

OutputLineParser::Result JavaParser::handleLine(const QString &line, OutputFormat type)
{
    Q_UNUSED(type);
    static const QRegularExpression javaRegExp("^(.*\\[javac\\]\\s)(.*\\.java):(\\d+):(.*)$");

    const QRegularExpressionMatch match = javaRegExp.match(line);
    if (!match.hasMatch())
        return Status::NotHandled;

    bool ok;
    int lineno = match.captured(3).toInt(&ok);
    if (!ok)
        lineno = -1;
    FilePath file = FilePath::fromUserInput(match.captured(2));
    if (file.isChildOf(m_buildDirectory)) {
        FilePath relativePath = file.relativeChildPath(m_buildDirectory);
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

} // Android::Internal
