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

#include "blackberryndkprocess.h"
#include "blackberryconfiguration.h"

#include <utils/hostosinfo.h>

#include <QTextStream>

namespace Qnx {
namespace Internal {

BlackBerryNdkProcess::BlackBerryNdkProcess(const QString &command, QObject *parent) :
    QObject(parent),
    m_process(new QProcess(this)),
    m_command(command)
{
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(processFinished()));
    connect(m_process, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(processError(QProcess::ProcessError)));
}

QString BlackBerryNdkProcess::command() const
{
    QString command = BlackBerryConfiguration::instance()
        .qnxEnv().value(QLatin1String("QNX_HOST"))
        + (QLatin1String("/usr/bin/")) + m_command;

    if (Utils::HostOsInfo::isWindowsHost())
        command += QLatin1String(".bat");

    return command;
}

void BlackBerryNdkProcess::start(const QStringList &arguments)
{
    if (m_process->state() != QProcess::NotRunning)
        return;

    m_process->start(command(), arguments);
}

void BlackBerryNdkProcess::addErrorStringMapping(
        const QString &message, int errorCode)
{
    m_errorStringMap.insert(message, errorCode);
}

void BlackBerryNdkProcess::processFinished()
{
    QTextStream processOutput(m_process);
    if (m_process->exitCode() == 0) {
        while (!processOutput.atEnd())
            processData(processOutput.readLine());

        emit finished(Success);
        return;
    }

    int returnStatus = UnknownError;
    while (!processOutput.atEnd()) {
        returnStatus = errorLineToReturnStatus(processOutput.readLine());
        if (returnStatus >= 0)
            break;
    }

    emit finished(returnStatus);
}

void BlackBerryNdkProcess::processError(QProcess::ProcessError error)
{
    int errorCode;

    switch (error) {
    case QProcess::FailedToStart:
        errorCode = FailedToStartInferiorProcess;
        break;
    case QProcess::Timedout:
        errorCode = InferiorProcessTimedOut;
        break;
    case QProcess::Crashed:
        errorCode = InferiorProcessCrashed;
        break;
    case QProcess::WriteError:
        errorCode = InferiorProcessWriteError;
        break;
    case QProcess::ReadError:
        errorCode = InferiorProcessReadError;
        break;
    case QProcess::UnknownError:
    default:
        errorCode = UnknownError;
        break;
    }

    emit finished(errorCode);
}

int BlackBerryNdkProcess::errorLineToReturnStatus(const QString &line) const
{
    typedef QMap<QString, int>::ConstIterator ConstIt;

    const ConstIt cend = m_errorStringMap.constEnd();
    for (ConstIt it = m_errorStringMap.constBegin(); it != cend; ++it) {
        if (line.contains(it.key()))
            return it.value();
    }

    return -1;
}

void BlackBerryNdkProcess::processData(const QString &line)
{
    Q_UNUSED(line);
}

} // namespace Internal
} // namespace Qnx
