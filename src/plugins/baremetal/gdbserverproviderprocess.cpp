/****************************************************************************
**
** Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gdbserverproviderprocess.h"

#include <projectexplorer/devicesupport/idevice.h>

#include <utils/environment.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QStringList>

namespace BareMetal {
namespace Internal {

GdbServerProviderProcess::GdbServerProviderProcess(
        const QSharedPointer<const ProjectExplorer::IDevice> &device,
        QObject *parent)
    : ProjectExplorer::DeviceProcess(device, parent)
    , m_process(new QProcess(this))
{
    connect(m_process, SIGNAL(error(QProcess::ProcessError)),
            SIGNAL(error(QProcess::ProcessError)));
    connect(m_process, SIGNAL(finished(int)), SIGNAL(finished()));

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &ProjectExplorer::DeviceProcess::readyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &ProjectExplorer::DeviceProcess::readyReadStandardError);
    connect(m_process, &QProcess::started,
            this, &ProjectExplorer::DeviceProcess::started);
}

void GdbServerProviderProcess::start(const QString &executable, const QStringList &arguments)
{
    QTC_ASSERT(m_process->state() == QProcess::NotRunning, return);
    m_process->start(executable, arguments);
}

void GdbServerProviderProcess::interrupt()
{
    device()->signalOperation()->interruptProcess(Utils::qPidToPid(m_process->pid()));
}

void GdbServerProviderProcess::terminate()
{
    m_process->terminate();
}

void GdbServerProviderProcess::kill()
{
    m_process->kill();
}

QProcess::ProcessState GdbServerProviderProcess::state() const
{
    return m_process->state();
}

QProcess::ExitStatus GdbServerProviderProcess::exitStatus() const
{
    return m_process->exitStatus();
}

int GdbServerProviderProcess::exitCode() const
{
    return m_process->exitCode();
}

QString GdbServerProviderProcess::errorString() const
{
    return m_process->errorString();
}

Utils::Environment GdbServerProviderProcess::environment() const
{
    return Utils::Environment(m_process->processEnvironment().toStringList());
}

void GdbServerProviderProcess::setEnvironment(const Utils::Environment &env)
{
    m_process->setProcessEnvironment(env.toProcessEnvironment());
}

void GdbServerProviderProcess::setWorkingDirectory(const QString &dir)
{
    m_process->setWorkingDirectory(dir);
}

QByteArray GdbServerProviderProcess::readAllStandardOutput()
{
    return m_process->readAllStandardOutput();
}

QByteArray GdbServerProviderProcess::readAllStandardError()
{
    return m_process->readAllStandardError();
}

qint64 GdbServerProviderProcess::write(const QByteArray &data)
{
    return m_process->write(data);
}

} // namespace Internal
} // namespace BareMetal
