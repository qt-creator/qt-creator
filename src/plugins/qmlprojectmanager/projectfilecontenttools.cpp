/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "projectfilecontenttools.h"

#include <projectexplorer/project.h>

#include <QRegularExpression>
#include <QRegularExpressionMatch>

namespace QmlProjectManager {
namespace ProjectFileContentTools {

QRegularExpression qdsVerRegexp(R"x(qdsVersion: "(.*)")x");

const Utils::FilePaths rootCmakeFiles(ProjectExplorer::Project *project)
{
    if (!project)
        project = ProjectExplorer::SessionManager::startupProject();
    if (!project)
        return {};
    return project->projectDirectory().dirEntries({QList<QString>("CMakeLists.txt"), QDir::Files});
}

const QString readFileContents(const Utils::FilePath &filePath)
{
    Utils::FileReader reader;
    if (!reader.fetch(filePath))
        return {};
    return QString::fromUtf8(reader.data());
}

const QString qdsVersion(const Utils::FilePath &projectFilePath)
{
    const QString projectFileContent = readFileContents(projectFilePath);
    QRegularExpressionMatch match = qdsVerRegexp.match(projectFileContent);
    if (!match.hasMatch())
        return {};
    QString version = match.captured(1);
    return version.isEmpty() ? QObject::tr("Unknown") : version;
}

QRegularExpression qt6Regexp("(qt6project:)\\s*\"*(true|false)\"*", QRegularExpression::CaseInsensitiveOption);

const QString qtVersion(const Utils::FilePath &projectFilePath)
{
    const QString defaultReturn = QObject::tr("Unknown");
    const QString data = readFileContents(projectFilePath);
    QRegularExpressionMatch match = qt6Regexp.match(data);
    if (!match.hasMatch())
        return defaultReturn;
    return match.captured(2).contains("true", Qt::CaseInsensitive)
            ? QObject::tr("Qt6 or later")
            : QObject::tr("Qt5 or earlier");
}

bool isQt6Project(const Utils::FilePath &projectFilePath)
{
    const QString data = readFileContents(projectFilePath);
    QRegularExpressionMatch match = qt6Regexp.match(data);
    if (!match.hasMatch())
        return false;
    return match.captured(2).contains("true", Qt::CaseInsensitive);
}

const QString getMainQmlFile(const Utils::FilePath &projectFilePath)
{
    const QString defaultReturn = "content/App.qml";
    const QString data = readFileContents(projectFilePath);
    QRegularExpression regexp(R"x(mainFile: "(.*)")x");
    QRegularExpressionMatch match = regexp.match(data);
    if (!match.hasMatch())
        return defaultReturn;
    return match.captured(1);
}

const QString appQmlFile(const Utils::FilePath &projectFilePath)
{
    return projectFilePath.toFileInfo().dir().absolutePath() + "/" +  getMainQmlFile(projectFilePath);
}

const Resolution resolutionFromConstants(const Utils::FilePath &projectFilePath)
{
    const QFileInfo fileInfo = projectFilePath.toFileInfo();
    const QString fileName = fileInfo.dir().absolutePath()
            + "/"  + "imports" + "/" + fileInfo.baseName() + "/Constants.qml";
    Utils::FileReader reader;
    if (!reader.fetch(Utils::FilePath::fromString(fileName)))
        return {};
    const QByteArray data = reader.data();
    const QRegularExpression regexpWidth(R"x(readonly\s+property\s+int\s+width:\s+(\d*))x");
    const QRegularExpression regexpHeight(R"x(readonly\s+property\s+int\s+height:\s+(\d*))x");
    int width = -1;
    int height = -1;
    QRegularExpressionMatch match = regexpHeight.match(QString::fromUtf8(data));
    if (match.hasMatch())
        height = match.captured(1).toInt();
    match = regexpWidth.match(QString::fromUtf8(data));
    if (match.hasMatch())
        width = match.captured(1).toInt();
    if (width > 0 && height > 0)
        return {width, height};
    return {};
}

} //ProjectFileContentTools
} //QmlProjectManager
