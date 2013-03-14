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

#ifndef QNX_INTERNAL_BLACKBERRYDEVICECONNECTION_H
#define QNX_INTERNAL_BLACKBERRYDEVICECONNECTION_H

#include <QObject>

#include <coreplugin/id.h>
#include <projectexplorer/devicesupport/idevice.h>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Qnx {
namespace Internal {

class BlackBerryDeviceConnection : public QObject
{
    Q_OBJECT
public:
    enum State {
        Disconnected,
        Connecting,
        Connected
    };

    explicit BlackBerryDeviceConnection();

    void connectDevice(const ProjectExplorer::IDevice::ConstPtr &device);
    void disconnectDevice();

    QString host() const;
    State connectionState();

    QString messageLog() const;

signals:
    void deviceAboutToConnect();
    void deviceConnected();
    void deviceDisconnected();

    void processOutput(const QString &output);

private slots:
    void processFinished();
    void readStandardOutput();
    void readStandardError();

private:
    QString m_host;
    State m_connectionState;

    QString m_messageLog;

    QProcess *m_process;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYDEVICECONNECTION_H
