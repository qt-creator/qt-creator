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

#include "tcpportsgatherer.h"
#include "qtcassert.h"

#include <utils/hostosinfo.h>

#include <QDebug>
#include <QFile>
#include <QProcess>

#ifdef Q_OS_WIN
#include <QLibrary>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif

#if defined(Q_OS_WIN) && defined(Q_CC_MINGW)

// Missing declarations for MinGW 32.
#if __GNUC__ == 4 && (!defined(__MINGW64_VERSION_MAJOR) || __MINGW64_VERSION_MAJOR < 2)
typedef enum { } MIB_TCP_STATE;
#endif

typedef struct _MIB_TCP6ROW {
    MIB_TCP_STATE State;
    IN6_ADDR LocalAddr;
    DWORD dwLocalScopeId;
    DWORD dwLocalPort;
    IN6_ADDR RemoteAddr;
    DWORD dwRemoteScopeId;
    DWORD dwRemotePort;
} MIB_TCP6ROW, *PMIB_TCP6ROW;

typedef struct _MIB_TCP6TABLE {
    DWORD dwNumEntries;
    MIB_TCP6ROW table[ANY_SIZE];
} MIB_TCP6TABLE, *PMIB_TCP6TABLE;

#endif // defined(Q_OS_WIN) && defined(Q_CC_MINGW)

namespace Utils {
namespace Internal {

class TcpPortsGathererPrivate
{
public:
    TcpPortsGathererPrivate()
        : protocol(QAbstractSocket::UnknownNetworkLayerProtocol) {}

    QAbstractSocket::NetworkLayerProtocol protocol;
    QSet<int> usedPorts;

    void updateWin();
    void updateLinux();
    void updateNetstat();
};

#ifdef Q_OS_WIN
template <typename Table >
QSet<int> usedTcpPorts(ULONG (__stdcall *Func)(Table*, PULONG, BOOL))
{
    Table *table = static_cast<Table*>(malloc(sizeof(Table)));
    DWORD dwSize = sizeof(Table);

    // get the necessary size into the dwSize variable
    DWORD dwRetVal = Func(table, &dwSize, false);
    if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) {
        free(table);
        table = static_cast<Table*>(malloc(dwSize));
    }

    // get the actual data
    QSet<int> result;
    dwRetVal = Func(table, &dwSize, false);
    if (dwRetVal == NO_ERROR) {
        for (quint32 i = 0; i < table->dwNumEntries; i++) {
            quint16 port = ntohs(table->table[i].dwLocalPort);
            if (!result.contains(port))
                result.insert(port);
        }
    } else {
        qWarning() << "TcpPortsGatherer: GetTcpTable failed with" << dwRetVal;
    }

    free(table);
    return result;
}
#endif

void TcpPortsGathererPrivate::updateWin()
{
#ifdef Q_OS_WIN
    QSet<int> ports;

    if (protocol == QAbstractSocket::IPv4Protocol) {
        ports.unite(usedTcpPorts<MIB_TCPTABLE>(GetTcpTable));
    } else {
        //Dynamically load symbol for GetTcp6Table for systems that dont have support for IPV6,
        //eg Windows XP
        typedef ULONG (__stdcall *GetTcp6TablePtr)(PMIB_TCP6TABLE, PULONG, BOOL);
        static GetTcp6TablePtr getTcp6TablePtr = 0;

        if (!getTcp6TablePtr)
            getTcp6TablePtr = (GetTcp6TablePtr)QLibrary::resolve(QLatin1String("Iphlpapi.dll"),
                                                                 "GetTcp6Table");

        if (getTcp6TablePtr && (protocol == QAbstractSocket::IPv6Protocol)) {
            ports.unite(usedTcpPorts<MIB_TCP6TABLE>(getTcp6TablePtr));
        } else if (protocol == QAbstractSocket::UnknownNetworkLayerProtocol) {
            ports.unite(usedTcpPorts<MIB_TCPTABLE>(GetTcpTable));
            if (getTcp6TablePtr)
                ports.unite(usedTcpPorts<MIB_TCP6TABLE>(getTcp6TablePtr));
        }
    }

    foreach (int port, ports)
        usedPorts.insert(port);
#endif
    Q_UNUSED(protocol);
}

void TcpPortsGathererPrivate::updateLinux()
{
    QStringList filePaths;
    const QString tcpFile = QLatin1String("/proc/net/tcp");
    const QString tcp6File = QLatin1String("/proc/net/tcp6");
    if (protocol == QAbstractSocket::IPv4Protocol)
        filePaths << tcpFile;
    else if (protocol == QAbstractSocket::IPv6Protocol)
        filePaths << tcp6File;
    else
        filePaths << tcpFile << tcp6File;

    foreach (const QString &filePath, filePaths) {
        QFile file(filePath);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            qWarning() << "TcpPortsGatherer: Cannot open file"
                       << filePath << ":" << file.errorString();
            continue;
        }

        if (file.atEnd()) // read first line describing the output
            file.readLine();

        static QRegExp pattern(QLatin1String("^\\s*" // start of line, whitespace
                                             "\\d+:\\s*" // integer, colon, space
                                             "[0-9A-Fa-f]+:" // hexadecimal number (ip), colon
                                             "([0-9A-Fa-f]+)"  // hexadecimal number (port!)
                                             ));
        while (!file.atEnd()) {
            QByteArray line = file.readLine();
            if (pattern.indexIn(QLatin1String(line)) != -1) {
                bool isNumber;
                quint16 port = pattern.cap(1).toUShort(&isNumber, 16);
                QTC_ASSERT(isNumber, continue);
                usedPorts.insert(port);
            } else {
                qWarning() << "TcpPortsGatherer: File" << filePath << "has unexpected format.";
                continue;
            }
        }
    }
}

// Only works with FreeBSD version of netstat like we have on Mac OS X
void TcpPortsGathererPrivate::updateNetstat()
{
    QStringList netstatArgs;

    netstatArgs.append(QLatin1String("-a"));     // show also sockets of server processes
    netstatArgs.append(QLatin1String("-n"));     // show network addresses as numbers
    netstatArgs.append(QLatin1String("-p"));
    netstatArgs.append(QLatin1String("tcp"));
    if (protocol != QAbstractSocket::UnknownNetworkLayerProtocol) {
        netstatArgs.append(QLatin1String("-f")); // limit to address family
        if (protocol == QAbstractSocket::IPv4Protocol)
            netstatArgs.append(QLatin1String("inet"));
        else
            netstatArgs.append(QLatin1String("inet6"));
    }

    QProcess netstatProcess;
    netstatProcess.start(QLatin1String("netstat"), netstatArgs);
    if (!netstatProcess.waitForFinished(30000)) {
        qWarning() << "TcpPortsGatherer: netstat did not return in time.";
        return;
    }

    QList<QByteArray> output = netstatProcess.readAllStandardOutput().split('\n');
    foreach (const QByteArray &line, output) {
        static QRegExp pattern(QLatin1String("^tcp[46]+" // "tcp", followed by "4", "6", "46"
                                             "\\s+\\d+"           // whitespace, number (Recv-Q)
                                             "\\s+\\d+"           // whitespace, number (Send-Q)
                                             "\\s+(\\S+)"));       // whitespace, Local Address
        if (pattern.indexIn(QLatin1String(line)) != -1) {
            QString localAddress = pattern.cap(1);

            // Examples of local addresses:
            // '*.56501' , '*.*' 'fe80::1%lo0.123'
            int portDelimiterPos = localAddress.lastIndexOf(QLatin1Char('.'));
            if (portDelimiterPos == -1)
                continue;

            localAddress = localAddress.mid(portDelimiterPos + 1);
            bool isNumber;
            quint16 port = localAddress.toUShort(&isNumber);
            if (!isNumber)
                continue;

            usedPorts.insert(port);
        }
    }
}

} // namespace Internal


/*!
  \class Utils::TcpPortsGatherer

  \brief Gather the list of local TCP ports already in use.

  Query the system for the list of local TCP ports already in use. This information can be used
  to select a port for use in a range.
*/

TcpPortsGatherer::TcpPortsGatherer()
    : d(new Internal::TcpPortsGathererPrivate())
{
}

TcpPortsGatherer::~TcpPortsGatherer()
{
    delete d;
}

void TcpPortsGatherer::update(QAbstractSocket::NetworkLayerProtocol protocol)
{
    d->protocol = protocol;
    d->usedPorts.clear();

    if (Utils::HostOsInfo::isWindowsHost())
        d->updateWin();
    else if (Utils::HostOsInfo::isLinuxHost())
        d->updateLinux();
    else
        d->updateNetstat();
}

QList<int> TcpPortsGatherer::usedPorts() const
{
    return d->usedPorts.values();
}

/*!
  Select a port out of \a freePorts that is not yet used.

  Returns the port, or -1 if no free port is available.
  */
int TcpPortsGatherer::getNextFreePort(PortList *freePorts) const
{
    QTC_ASSERT(freePorts, return -1);
    while (freePorts->hasMore()) {
        const int port = freePorts->getNext();
        if (!d->usedPorts.contains(port))
            return port;
    }
    return -1;
}

} // namespace Utils
