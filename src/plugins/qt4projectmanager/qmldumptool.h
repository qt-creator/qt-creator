/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLDUMPTOOL_H
#define QMLDUMPTOOL_H

#include "qt4projectmanager_global.h"

#include <utils/buildablehelperlibrary.h>

namespace Utils {
    class Environment;
}

namespace ProjectExplorer {
    class Project;
}

namespace Qt4ProjectManager {
class QtVersion;

class QT4PROJECTMANAGER_EXPORT QmlDumpTool : public Utils::BuildableHelperLibrary
{
public:
    static bool canBuild(const QtVersion *qtVersion);
    static QString toolForProject(ProjectExplorer::Project *project, bool debugDump);
    static QString toolByInstallData(const QString &qtInstallData, const QString &qtInstallHeaders,
                                     bool debugDump);
    static QStringList locationsByInstallData(const QString &qtInstallData, bool debugDump);

    // Build the helpers and return the output log/errormessage.
    static bool build(const QString &directory, const QString &makeCommand,
                      const QString &qmakeCommand, const QString &mkspec,
                      const Utils::Environment &env, const QString &targetMode,
                      const QStringList &qmakeArguments, QString *output,  QString *errorMessage);

    // Copy the source files to a target location and return the chosen target location.
    static QString copy(const QString &qtInstallData, QString *errorMessage);

    static void pathAndEnvironment(ProjectExplorer::Project *project, bool preferDebug,
                                   QString *path, Utils::Environment *env);

private:
    static QStringList installDirectories(const QString &qtInstallData);

};

} // namespace

#endif // QMLDUMPTOOL_H
