/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmldumptool.h"
#include "qtkitinformation.h"
#include "qtsupportconstants.h"
#include "qtversionmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/project.h>
#include <utils/runextensions.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QHash>
#include <QStandardPaths>

namespace QtSupport {

static inline QStringList validPrebuiltFilenames(bool debugBuild)
{
    QStringList list = QStringList(QLatin1String("qmlplugindump"));
    list.append(QLatin1String("qmlplugindump.app/Contents/MacOS/qmlplugindump"));
    if (debugBuild)
        list.prepend(QLatin1String("qmlplugindumpd.exe"));
    else
        list.prepend(QLatin1String("qmlplugindump.exe"));
    return list;
}

QString QmlDumpTool::toolForVersion(const BaseQtVersion *version, bool debugDump)
{
    if (version) {
        const QString qtInstallBins = version->qmakeProperty("QT_INSTALL_BINS");
        return toolForQtPaths(qtInstallBins, debugDump);
    }

    return QString();
}

QString QmlDumpTool::toolForQtPaths(const QString &qtInstallBins,
                                    bool debugDump)
{
    if (!Core::ICore::instance())
        return QString();

    // check for prebuilt binary first
    QFileInfo fileInfo;
    if (getHelperFileInfoFor(validPrebuiltFilenames(debugDump), qtInstallBins + QLatin1Char('/'), &fileInfo))
        return fileInfo.absoluteFilePath();

    return QString();
}

void QmlDumpTool::pathAndEnvironment(const ProjectExplorer::Kit *k, bool preferDebug,
                                     QString *dumperPath, Utils::Environment *env)
{
    if (!k)
        return;

    const BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
    if (version && !version->hasQmlDump())
        return;

    QString path;

    path = toolForVersion(version, preferDebug);
    if (path.isEmpty())
        path = toolForVersion(version, !preferDebug);

    if (!path.isEmpty()) {
        QFileInfo qmldumpFileInfo(path);
        if (!qmldumpFileInfo.exists()) {
            qWarning() << "QmlDumpTool::qmlDumpPath: qmldump executable does not exist at" << path;
            path.clear();
        } else if (!qmldumpFileInfo.isFile()) {
            qWarning() << "QmlDumpTool::qmlDumpPath: " << path << " is not a file";
            path.clear();
        }
    }

    if (!path.isEmpty() && version && dumperPath && env) {
        *dumperPath = path;
        k->addToEnvironment(*env);
    }
}

} // namespace QtSupport

