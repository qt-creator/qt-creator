/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "networkaccessmanager.h"

#include <QCoreApplication>
#include <QLocale>
#include <QNetworkReply>

#ifdef Q_OS_UNIX
#include <sys/utsname.h>
#endif

/*!
   \class Utils::NetworkManager

    \brief The NetworkManager class provides a network access manager for use
    with \QC.

   Common initialization, \QC User Agent.

   Preferably, the instance returned by NetworkAccessManager::instance() should be used for the main
   thread. The constructor is provided only for multithreaded use.
 */

namespace Utils {

static NetworkAccessManager *namInstance = 0;

void cleanupNetworkAccessManager()
{
    delete namInstance;
    namInstance = 0;
}

NetworkAccessManager *NetworkAccessManager::instance()
{
    if (!namInstance) {
        namInstance = new NetworkAccessManager;
        qAddPostRoutine(cleanupNetworkAccessManager);
    }
    return namInstance;
}

static const QString getOsString()
{
    QString osString;
#if defined(Q_OS_WIN)
    switch (QSysInfo::WindowsVersion) {
    case (QSysInfo::WV_4_0):
        osString += QLatin1String("WinNT4.0");
        break;
    case (QSysInfo::WV_5_0):
        osString += QLatin1String("Windows NT 5.0");
        break;
    case (QSysInfo::WV_5_1):
        osString += QLatin1String("Windows NT 5.1");
        break;
    case (QSysInfo::WV_5_2):
        osString += QLatin1String("Windows NT 5.2");
        break;
    case (QSysInfo::WV_6_0):
        osString += QLatin1String("Windows NT 6.0");
        break;
    case (QSysInfo::WV_6_1):
        osString += QLatin1String("Windows NT 6.1");
        break;
    default:
        osString += QLatin1String("Windows NT (Unknown)");
        break;
    }
#elif defined (Q_OS_MAC)
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
        osString += QLatin1String("PPC ");
    else
        osString += QLatin1String("Intel ");
    osString += QLatin1String("Mac OS X ");
    switch (QSysInfo::MacintoshVersion) {
    case (QSysInfo::MV_10_3):
        osString += QLatin1String("10_3");
        break;
    case (QSysInfo::MV_10_4):
        osString += QLatin1String("10_4");
        break;
    case (QSysInfo::MV_10_5):
        osString += QLatin1String("10_5");
        break;
    case (QSysInfo::MV_10_6):
        osString += QLatin1String("10_6");
        break;
    default:
        osString += QLatin1String("(Unknown)");
        break;
    }
#elif defined (Q_OS_UNIX)
    struct utsname uts;
    if (uname(&uts) == 0) {
        osString += QLatin1String(uts.sysname);
        osString += QLatin1Char(' ');
        osString += QLatin1String(uts.release);
    } else {
        osString += QLatin1String("Unix (Unknown)");
    }
#else
    osString = QLatin1String("Unknown OS");
#endif
    return osString;
}


NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{

}

QNetworkReply* NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    QString agentStr = QString::fromLatin1("%1/%2 (QNetworkAccessManager %3; %4; %5; %6 bit)")
                    .arg(QCoreApplication::applicationName(),
                         QCoreApplication::applicationVersion(),
                         QLatin1String(qVersion()),
                         getOsString(), QLocale::system().name())
                    .arg(QSysInfo::WordSize);
    QNetworkRequest req(request);
    req.setRawHeader("User-Agent", agentStr.toLatin1());
    return QNetworkAccessManager::createRequest(op, req, outgoingData);
}


} // namespace utils
