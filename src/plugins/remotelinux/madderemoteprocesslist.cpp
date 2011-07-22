/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
#include "madderemoteprocesslist.h"

#include <remotelinux/linuxdeviceconfiguration.h>

#include <QtCore/QString>

namespace RemoteLinux {
namespace Internal {
namespace {

const char FremantleLineSeparator1[] = "---";
const char FremantleLineSeparator2[] = "QTCENDOFLINE---";

} // anonymous namespace

MaddeRemoteProcessList::MaddeRemoteProcessList(const LinuxDeviceConfiguration::ConstPtr &devConfig,
    QObject *parent) : GenericRemoteLinuxProcessList(devConfig, parent)
{
}

QString MaddeRemoteProcessList::listProcessesCommandLine() const
{
    // The ps command on Fremantle ignores all command line options, so
    // we have to collect the information in /proc manually.
    if (deviceConfiguration()->osType() == LinuxDeviceConfiguration::Maemo5OsType) {
        return QLatin1String("sep1=") + QLatin1String(FremantleLineSeparator1) + QLatin1Char(';')
            + QLatin1String("sep2=") + QLatin1String(FremantleLineSeparator2) + QLatin1Char(';')
            + QLatin1String("pidlist=`ls /proc |grep -E '^[[:digit:]]+$' |sort -n`; "
                  "for pid in $pidlist;"
                   "do "
                   "    echo -n \"$pid \";"
                   "    tr '\\0' ' ' < /proc/$pid/cmdline;"
                   "    echo -n \"$sep1$sep2\";"
                   "done;"
                   "echo ''");
    }
    return GenericRemoteLinuxProcessList::listProcessesCommandLine();
}

QList<AbstractRemoteLinuxProcessList::RemoteProcess> MaddeRemoteProcessList::buildProcessList(const QString &listProcessesReply) const
{
    QString adaptedReply = listProcessesReply;
    if (deviceConfiguration()->osType() == LinuxDeviceConfiguration::Maemo5OsType) {
        adaptedReply.replace(QLatin1String(FremantleLineSeparator1)
            + QLatin1String(FremantleLineSeparator2), QLatin1String("\n"));
        adaptedReply.prepend(QLatin1String("dummy\n"));
    }

    return GenericRemoteLinuxProcessList::buildProcessList(adaptedReply);
}

} // namespace Internal
} // namespace RemoteLinux
