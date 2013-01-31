/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QMLDUMPTOOL_H
#define QMLDUMPTOOL_H

#include "qtsupport_global.h"

#include <utils/buildablehelperlibrary.h>

namespace Utils {
    class Environment;
}

namespace ProjectExplorer {
    class Project;
    class ToolChain;
}

namespace QtSupport {
class BaseQtVersion;

class QTSUPPORT_EXPORT QmlDumpTool : public Utils::BuildableHelperLibrary
{
public:
    static bool canBuild(const BaseQtVersion *qtVersion, QString *reason = 0);
    static QString toolForVersion(BaseQtVersion *version, bool debugDump);
    static QString toolForQtPaths(const QString &qtInstallData,
                                 const QString &qtInstallBins,
                                 const QString &qtInstallHeaders,
                                 bool debugDump);
    static QStringList locationsByInstallData(const QString &qtInstallData, bool debugDump);

    // Build the helpers and return the output log/errormessage.
    static bool build(BuildHelperArguments arguments, QString *log, QString *errorMessage);

    // Copy the source files to a target location and return the chosen target location.
    static QString copy(const QString &qtInstallData, QString *errorMessage);

    static void pathAndEnvironment(ProjectExplorer::Project *project, BaseQtVersion *version,
                                   ProjectExplorer::ToolChain *toolChain,
                                   bool preferDebug, QString *path, Utils::Environment *env);

private:
    static QStringList installDirectories(const QString &qtInstallData);

};

} // namespace

#endif // QMLDUMPTOOL_H
