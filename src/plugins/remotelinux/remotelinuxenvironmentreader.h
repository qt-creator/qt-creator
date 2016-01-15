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

#ifndef REMOTELINUXENVIRONMENTREADER_H
#define REMOTELINUXENVIRONMENTREADER_H

#include <utils/environment.h>

#include <QObject>

namespace ProjectExplorer {
class DeviceProcess;
class Kit;
class RunConfiguration;
}

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxEnvironmentReader : public QObject
{
    Q_OBJECT

public:
    RemoteLinuxEnvironmentReader(ProjectExplorer::RunConfiguration *config, QObject *parent = 0);

    void start();
    void stop();

    Utils::Environment remoteEnvironment() const { return m_env; }

signals:
    void finished();
    void error(const QString &error);

private slots:
    void handleError();
    void handleCurrentDeviceConfigChanged();
    void remoteProcessFinished();

private:
    void setFinished();
    void destroyProcess();

    bool m_stop;
    Utils::Environment m_env;
    ProjectExplorer::Kit *m_kit;
    ProjectExplorer::DeviceProcess *m_deviceProcess;
};

} // namespace Internal
} // namespace RemoteLinux

#endif  // REMOTELINUXENVIRONMENTREADER_H
