/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qnxutils.h"

#include "qnxabstractqtversion.h"

#include <QDir>

using namespace Qnx;
using namespace Qnx::Internal;

QString QnxUtils::addQuotes(const QString &string)
{
    return QLatin1Char('"') + string + QLatin1Char('"');
}

Qnx::QnxArchitecture QnxUtils::cpudirToArch(const QString &cpuDir)
{
    if (cpuDir == QLatin1String("x86"))
        return Qnx::X86;
    else if (cpuDir == QLatin1String("armle-v7"))
        return Qnx::ArmLeV7;
    else
        return Qnx::UnknownArch;
}

QStringList QnxUtils::searchPaths(QnxAbstractQtVersion *qtVersion)
{
    const QDir pluginDir(qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_PLUGINS")));
    const QStringList pluginSubDirs = pluginDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    QStringList searchPaths;

    Q_FOREACH (const QString &dir, pluginSubDirs) {
        searchPaths << qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_PLUGINS"))
                       + QLatin1Char('/') + dir;
    }

    searchPaths << qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_LIBS"));
    searchPaths << qtVersion->qnxTarget() + QLatin1Char('/') + qtVersion->archString().toLower()
                   + QLatin1String("/lib");
    searchPaths << qtVersion->qnxTarget() + QLatin1Char('/') + qtVersion->archString().toLower()
                   + QLatin1String("/usr/lib");

    return searchPaths;
}

QString QnxUtils::applicationKillCommand(const QString &applicationFilePath)
{
    return QString::fromLatin1("for PID in $(ps -f -o pid,comm | grep %1 | awk '/%1/ {print $1}'); "
            "do "
            "kill $PID; sleep 1; kill -9 $PID; "
            "done").arg(applicationFilePath);
}
