/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "s60projectchecker.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <qt4projectmanager/qt-s60/s60manager.h>
#include <qt4projectmanager/qt4project.h>

#include <QtCore/QCoreApplication>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

QList<ProjectExplorer::Task>
S60ProjectChecker::reportIssues(const QString &proFile, const QtVersion *version)
{
    QList<ProjectExplorer::Task> results;
    const QString epocRootDir = QDir(Internal::S60Manager::instance()->deviceForQtVersion(version).epocRoot).absolutePath();
    QFileInfo cppheader(epocRootDir + QLatin1String("/epoc32/include/stdapis/string.h"));
#if defined (Q_OS_WIN)
    // Report an error if project- and epoc directory are on different drives:
    if (!epocRootDir.startsWith(proFile.left(3), Qt::CaseInsensitive)) {
        results.append(Task(Task::Error,
                            QCoreApplication::translate("ProjectExplorer::Internal::S60ProjectChecker",
                                                        "The Symbian SDK and the project sources must reside on the same drive."),
                            QString(), -1, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
#endif
    // Report an error if EPOC root is not set:
    if (epocRootDir.isEmpty() || !QDir(epocRootDir).exists()) {
        results.append(Task(Task::Error,
                            QCoreApplication::translate("ProjectExplorer::Internal::S60ProjectChecker",
                                                        "The Symbian SDK was not found for Qt version %1.").arg(version->displayName()),
                            QString(), -1, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    if (!cppheader.exists()) {
        results.append(Task(Task::Error,
                            QCoreApplication::translate("ProjectExplorer::Internal::S60ProjectChecker",
                                                        "The \"Open C/C++ plugin\" is not installed in the Symbian SDK or the Symbian SDK path is misconfigured for Qt version %1.").arg(version->displayName()),
                            QString(), -1, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }

    // Warn of strange characters in project name and path:
    const QString projectName = proFile.mid(proFile.lastIndexOf(QLatin1Char('/')) + 1);
    QString projectPath = proFile.left(proFile.lastIndexOf(QLatin1Char('/')));
#if defined (Q_OS_WIN)
    if (projectPath.at(1) == QChar(':') && projectPath.at(0).toUpper() >= QChar('A') && projectPath.at(0).toUpper() <= QChar('Z'))
        projectPath = projectPath.mid(2);
#endif
    if (projectPath.contains(QLatin1Char(' '))) {
        results.append(Task(Task::Warning,
                            QCoreApplication::translate("ProjectExplorer::Internal::S60ProjectChecker",
                                                        "The Symbian toolchain does not handle spaces "
                                                        "in the project path '%1'.").arg(projectPath),
                            QString(), -1, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    if (projectName.contains(QRegExp("[^a-zA-Z0-9.]"))) {
        results.append(Task(Task::Warning,
                            QCoreApplication::translate("ProjectExplorer::Internal::S60ProjectChecker",
                                                        "The Symbian toolchain does not handle special "
                                                        "characters in the project name '%1' well.")
                            .arg(projectName),
                            QString(), -1, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    return results;
}

QList<ProjectExplorer::Task>
S60ProjectChecker::reportIssues(const Qt4Project *project, const QtVersion *version)
{
    QString proFile = project->file()->fileName();
    return reportIssues(proFile, version);
}
