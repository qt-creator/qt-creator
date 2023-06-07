// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "networkaccessmanager.h"

#include <QCoreApplication>
#include <QLocale>
#include <QNetworkReply>
#include <QSysInfo>

/*!
   \class Utils::NetworkAccessManager
   \inmodule QtCreator

    \brief The NetworkAccessManager class provides a network access manager for use
    with \QC.

   Common initialization, \QC User Agent.

   Preferably, the instance returned by NetworkAccessManager::instance() should be used for the main
   thread. The constructor is provided only for multithreaded use.
 */

namespace Utils {

static NetworkAccessManager *namInstance = nullptr;

void cleanupNetworkAccessManager()
{
    delete namInstance;
    namInstance = nullptr;
}

/*!
    Returns a network access manager instance that should be used for the main
    thread.
*/
NetworkAccessManager *NetworkAccessManager::instance()
{
    if (!namInstance) {
        namInstance = new NetworkAccessManager;
        qAddPostRoutine(cleanupNetworkAccessManager);
    }
    return namInstance;
}

/*!
    Constructs a network access manager instance with the parent \a parent.
*/
NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{

}

/*!
    Creates \a request for the network access manager to perform the operation
    \a op on \a outgoingData.
*/
QNetworkReply* NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    QString agentStr = QString::fromLatin1("%1/%2 (QNetworkAccessManager %3; %4; %5; %6 bit)")
                    .arg(QCoreApplication::applicationName(),
                         QCoreApplication::applicationVersion(),
                         QLatin1String(qVersion()),
                         QSysInfo::prettyProductName(),
                         QLocale::system().name())
                    .arg(QSysInfo::WordSize);
    QNetworkRequest req(request);
    req.setRawHeader("User-Agent", agentStr.toLatin1());
    return QNetworkAccessManager::createRequest(op, req, outgoingData);
}

} // namespace utils
