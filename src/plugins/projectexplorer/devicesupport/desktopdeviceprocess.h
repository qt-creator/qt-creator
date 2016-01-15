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

#ifndef QTC_DESKTOPDEVICEPROCESS_H
#define QTC_DESKTOPDEVICEPROCESS_H

#include "deviceprocess.h"

namespace ProjectExplorer {
namespace Internal {
class ProcessHelper;

class DesktopDeviceProcess : public DeviceProcess
{
    Q_OBJECT
public:
    DesktopDeviceProcess(const QSharedPointer<const IDevice> &device, QObject *parent = 0);

    void start(const QString &executable, const QStringList &arguments);
    void interrupt();
    void terminate();
    void kill();

    QProcess::ProcessState state() const;
    QProcess::ExitStatus exitStatus() const;
    int exitCode() const;
    QString errorString() const;

    Utils::Environment environment() const;
    void setEnvironment(const Utils::Environment &env);

    void setWorkingDirectory(const QString &directory);

    QByteArray readAllStandardOutput();
    QByteArray readAllStandardError();

    qint64 write(const QByteArray &data);

private:
    QProcess * const m_process;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // Include guard
