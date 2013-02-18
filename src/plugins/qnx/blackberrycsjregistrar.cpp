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

#include "blackberrycsjregistrar.h"
#include "blackberryconfiguration.h"

#include <utils/hostosinfo.h>

#include <QProcess>
#include <QStringList>
#include <QString>

namespace Qnx {
namespace Internal {

BlackBerryCsjRegistrar::BlackBerryCsjRegistrar(QObject *parent) :
    QObject(parent),
    m_process(new QProcess(this))
{
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(processFinished()));
    connect(m_process, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(processError(QProcess::ProcessError)));
}

void BlackBerryCsjRegistrar::tryRegister(const QStringList &csjFiles,
        const QString &csjPin, const QString &cskPassword)
{
    if (m_process->state() != QProcess::NotRunning)
        return;

    QString command = BlackBerryConfiguration::instance()
        .qnxEnv().value(QLatin1String("QNX_HOST"))
        + (QLatin1String("/usr/bin/blackberry-signer"));

    if (Utils::HostOsInfo::isWindowsHost())
        command += QLatin1String(".bat");

    QStringList arguments;

    arguments << QLatin1String("-register")
              << QLatin1String("-cskpass")
              << cskPassword
              << QLatin1String("-csjpin")
              << csjPin
              << csjFiles;

    m_process->start(command, arguments);
}

void BlackBerryCsjRegistrar::processFinished()
{
    QByteArray result = m_process->readAllStandardOutput();

    if (result.contains("Successfully registered with server."))
        emit finished(RegisterSuccess, QString());
    else
        emit finished(Error, QLatin1String(result));
}

void BlackBerryCsjRegistrar::processError(QProcess::ProcessError error)
{
    QString errorMessage;

    switch (error) {
    case QProcess::FailedToStart:
        errorMessage = tr("Failed to start blackberry-signer process.");
        break;
    case QProcess::Timedout:
        errorMessage = tr("Process timed out.");
        break;
    case QProcess::Crashed:
        errorMessage = tr("Child process has crashed.");
        break;
    case QProcess::WriteError:
    case QProcess::ReadError:
        errorMessage = tr("Process I/O error.");
        break;
    case QProcess::UnknownError:
        errorMessage = tr("Unknown process error.");
        break;
    }

    emit finished(Error, errorMessage);
}

} // namespace Internal
} // namespace Qnx
