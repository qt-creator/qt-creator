// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectfilecontenttools.h"

#include "qmlprojectmanagertr.h"

#include <projectexplorer/project.h>

#include <QRegularExpression>
#include <QRegularExpressionMatch>

namespace QmlProjectManager {
namespace ProjectFileContentTools {

QRegularExpression qdsVerRegexp(R"x(qdsVersion: "(.*)")x");

const Utils::FilePaths rootCmakeFiles(ProjectExplorer::Project *project)
{
    if (!project)
        project = ProjectExplorer::ProjectManager::startupProject();
    if (!project)
        return {};
    return project->projectDirectory().dirEntries({QList<QString>({"CMakeLists.txt"}), QDir::Files});
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
    if (match.hasMatch()) {
        const QString version = match.captured(1);
        if (!version.isEmpty())
            return version;
    }

    return Tr::tr("Unknown");
}

QRegularExpression quickRegexp("(quickVersion:)\\s*\"(\\d+.\\d+)\"",
                               QRegularExpression::CaseInsensitiveOption);
QRegularExpression qt6Regexp("(qt6Project:)\\s*\"*(true|false)\"*",
                             QRegularExpression::CaseInsensitiveOption);

const QString qtVersion(const Utils::FilePath &projectFilePath)
{
    const QString defaultReturn = Tr::tr("Unknown");
    const QString data = readFileContents(projectFilePath);

    // First check if quickVersion is contained in the project file
    QRegularExpressionMatch match = quickRegexp.match(data);
    if (match.hasMatch())
        return QString("Qt %1").arg(match.captured(2));

    // If quickVersion wasn't found check for qt6Project
    match = qt6Regexp.match(data);
    if (match.hasMatch())
        return match.captured(2).contains("true", Qt::CaseInsensitive) ? Tr::tr("Qt 6")
                                                                       : Tr::tr("Qt 5");

    return defaultReturn;
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
