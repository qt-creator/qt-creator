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

#ifndef QTC_DEVICEPROCESS_H
#define QTC_DEVICEPROCESS_H

#include "../projectexplorer_export.h"

#include <QObject>
#include <QProcess>
#include <QSharedPointer>
#include <QStringList>

namespace Utils { class Environment; }

namespace ProjectExplorer {
class IDevice;

class PROJECTEXPLORER_EXPORT DeviceProcess : public QObject
{
    Q_OBJECT
public:
    virtual ~DeviceProcess();

    virtual void start(const QString &executable, const QStringList &arguments = QStringList()) = 0;
    virtual void interrupt() = 0;
    virtual void terminate() = 0;
    virtual void kill() = 0;

    virtual QProcess::ProcessState state() const = 0;
    virtual QProcess::ExitStatus exitStatus() const = 0;
    virtual int exitCode() const = 0;
    virtual QString errorString() const = 0;

    virtual Utils::Environment environment() const = 0;
    virtual void setEnvironment(const Utils::Environment &env) = 0;

    virtual void setWorkingDirectory(const QString &workingDirectory) = 0;

    virtual QByteArray readAllStandardOutput() = 0;
    virtual QByteArray readAllStandardError() = 0;

    virtual qint64 write(const QByteArray &data) = 0;

signals:
    void started();
    void finished();
    void error(QProcess::ProcessError error);

    void readyReadStandardOutput();
    void readyReadStandardError();

protected:
    DeviceProcess(const QSharedPointer<const IDevice> &device, QObject *parent = 0);
    QSharedPointer<const IDevice> device() const;

private:
    const QSharedPointer<const IDevice> m_device;
};

} // namespace ProjectExplorer

#endif // Include guard.
