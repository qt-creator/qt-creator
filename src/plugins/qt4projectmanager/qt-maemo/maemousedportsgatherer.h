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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MAEMOUSEDPORTSGATHERER_H
#define MAEMOUSEDPORTSGATHERER_H

#include "maemodeviceconfigurations.h"

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

namespace Utils {
class SshConnection;
class SshRemoteProcessRunner;
}

namespace Qt4ProjectManager {
namespace Internal {

class MaemoUsedPortsGatherer : public QObject
{
    Q_OBJECT
public:
    explicit MaemoUsedPortsGatherer(QObject *parent = 0);
    ~MaemoUsedPortsGatherer();
    void start(const QSharedPointer<Utils::SshConnection> &connection,
        const MaemoPortList &portList);
    void stop();
    int getNextFreePort(MaemoPortList *freePorts) const; // returns -1 if no more are left
    QList<int> usedPorts() const { return m_usedPorts; }

signals:
    void error(const QString &errMsg);
    void portListReady();

private slots:
    void handleConnectionError();
    void handleProcessClosed(int exitStatus);
    void handleRemoteStdOut(const QByteArray &output);
    void handleRemoteStdErr(const QByteArray &output);

private:
    void setupUsedPorts();

    QSharedPointer<Utils::SshRemoteProcessRunner> m_procRunner;
    QList<int> m_usedPorts;
    QByteArray m_remoteStdout;
    QByteArray m_remoteStderr;
    bool m_running;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOUSEDPORTSGATHERER_H
