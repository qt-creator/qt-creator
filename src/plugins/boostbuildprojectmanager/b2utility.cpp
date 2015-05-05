//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
// Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#include "b2utility.h"
// Qt Creator
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
// Qt
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QList>
#include <QRegExp>
#include <QSet>
#include <QString>
#include <QStringList>

namespace BoostBuildProjectManager {
namespace Utility {

QStringList readLines(QString const& filePath)
{
    QFileInfo const fileInfo(filePath);
    QStringList lines;

    QFile file(fileInfo.absoluteFilePath());
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);

        forever {
            QString line = stream.readLine();
            if (line.isNull())
                break;

            lines.append(line);
        }
    }

    return lines;
}

QStringList makeAbsolutePaths(QString const& basePath, QStringList const& paths)
{
    QDir const baseDir(basePath);

    QFileInfo fileInfo;
    QStringList absolutePaths;
    foreach (QString const& path, paths) {
        QString trimmedPath = path.trimmed();
        if (!trimmedPath.isEmpty()) {
            trimmedPath = Utils::FileName::fromUserInput(trimmedPath).toString();

            fileInfo.setFile(baseDir, trimmedPath);
            if (fileInfo.exists()) {
                QString const absPath = fileInfo.absoluteFilePath();
                Q_ASSERT(!absPath.isEmpty());
                absolutePaths.append(absPath);
            }
        }
    }
    absolutePaths.removeDuplicates();
    return absolutePaths;
}

QStringList& makeRelativePaths(QString const& basePath, QStringList& paths)
{
    QDir const baseDir(basePath);
    for (QStringList::iterator it = paths.begin(), end = paths.end(); it != end; ++it)
        *it = baseDir.relativeFilePath(*it);
    return paths;
}

QHash<QString, QStringList> sortFilesIntoPaths(QString const& basePath
                                             , QSet<QString> const& files)
{
    QHash<QString, QStringList> filesInPath;
    QDir const baseDir(basePath);

    foreach (QString const& absoluteFileName, files) {
        QFileInfo const fileInfo(absoluteFileName);
        Utils::FileName absoluteFilePath = Utils::FileName::fromString(fileInfo.path());
        QString relativeFilePath;

        if (absoluteFilePath.isChildOf(baseDir)) {
            relativeFilePath = absoluteFilePath.relativeChildPath(
                        Utils::FileName::fromString(basePath)).toString();
        } else {
            // `file' is not part of the project.
            relativeFilePath = baseDir.relativeFilePath(absoluteFilePath.toString());
            if (relativeFilePath.endsWith(QLatin1Char('/')))
                relativeFilePath.chop(1);
        }

        filesInPath[relativeFilePath].append(absoluteFileName);
    }
    return filesInPath;
}

// Parses Jamfile and looks for project rule to extract project name.
// Boost.Build project rule has the following syntax:
//      project id : attributes ;
// The project definition can span across multiple lines, including empty lines.
// but each syntax token must be separated with a whitespace..
QString parseJamfileProjectName(QString const& fileName)
{
    QString projectName;
    QFile file(fileName);
    if (file.exists()) {
        // Jamfile project rule tokens to search for
        QString const ruleBeg(QLatin1String("project"));
        QChar const ruleEnd(QLatin1Char(';'));
        QChar const attrSep(QLatin1Char(':'));
        QChar const tokenSep(QLatin1Char(' ')); // used to ensure tokens separation
        QString projectDef; // buffer for complete project definition

        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString const line(stream.readLine());
            if (projectDef.isEmpty() && line.trimmed().startsWith(ruleBeg))
                projectDef.append(line + tokenSep);
            else if (!projectDef.isEmpty())
                projectDef.append(line + tokenSep);

            if (projectDef.contains(attrSep) || projectDef.contains(ruleEnd))
                break;
        }

        if (!projectDef.isEmpty()) {
            QRegExp rx(QLatin1String("\\s*project\\s+([a-zA-Z\\-\\/]+)\\s+[\\:\\;]?"));
            rx.setMinimal(true);
            QTC_CHECK(rx.isValid());
            if (rx.indexIn(projectDef) > -1)
                projectName = rx.cap(1);
        }
    }
    return projectName;
}

} // namespace Utility
} // namespace BoostBuildProjectManager
