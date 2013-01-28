/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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
