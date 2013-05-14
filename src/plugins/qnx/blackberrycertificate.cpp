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

#include "blackberrycertificate.h"
#include "blackberryconfiguration.h"

#include <utils/hostosinfo.h>

#include <QProcess>
#include <QFile>
#include <QTextStream>

namespace Qnx {
namespace Internal {

BlackBerryCertificate::BlackBerryCertificate(const QString &fileName,
        const QString &author, const QString &storePass, QObject *parent) :
    QObject(parent),
    m_fileName(fileName),
    m_author(author),
    m_storePass(storePass),
    m_process(new QProcess(this))
{
}

void BlackBerryCertificate::load()
{
    if (m_process->state() != QProcess::NotRunning) {
        emit finished(BlackBerryCertificate::Busy);
        return;
    }
    QStringList arguments;

    arguments << QLatin1String("-keystore")
              << m_fileName
              << QLatin1String("-list")
              << QLatin1String("-verbose")
              << QLatin1String("-storepass")
              << m_storePass;

    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(loadFinished()));
    connect(m_process, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(processError()));

    m_process->start(command(), arguments);
}

void BlackBerryCertificate::store()
{
    if (m_process->state() != QProcess::NotRunning) {
        emit finished(BlackBerryCertificate::Busy);
        return;
    }

    QFile file(m_fileName);

    if (file.exists())
        file.remove();

    QStringList arguments;

    arguments << QLatin1String("-genkeypair")
              << QLatin1String("-storepass")
              << m_storePass
              << QLatin1String("-author")
              << m_author
              << QLatin1String("-keystore")
              << m_fileName;

    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(storeFinished(int)));
    connect(m_process, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(processError()));

    m_process->start(command(), arguments);
}

QString BlackBerryCertificate::fileName() const
{
    return m_fileName;
}

QString BlackBerryCertificate::author() const
{
    return m_author;
}

QString BlackBerryCertificate::id() const
{
    QString tmpId = fileName();
    return tmpId.replace(QLatin1Char('/'), QLatin1Char('-'));
}

void BlackBerryCertificate::storeFinished(int status)
{
    m_process->disconnect();

    if (status == 0)
        emit finished(BlackBerryCertificate::Success);
    else
        emit finished(BlackBerryCertificate::Error);
}

void BlackBerryCertificate::loadFinished()
{
    m_process->disconnect();

    ResultCode status = Error;

    QTextStream processOutput(m_process);

    while (!processOutput.atEnd()) {
        QString chunk = processOutput.readLine();

        if (chunk.contains(
                QLatin1String("Error: Failed to decrypt keystore, invalid password"))) {
            status = WrongPassword;
            break;
        } else if (chunk.startsWith(QLatin1String("Owner:"))) {
            chunk.remove(QLatin1String("Owner:"));
            m_author = chunk.remove(QLatin1String("CN=")).trimmed();
            status = Success;
            break;
        } else if (chunk.contains(QLatin1String("Subject Name:"))) {
            // this format is used by newer NDKs, the interesting data
            // comes on the next line
            chunk = processOutput.readLine();
            const QString token = QLatin1String("CommonName=");
            if (chunk.contains(token)) {
                m_author = chunk.remove(token).trimmed();
                status = Success;
            } else {
                status = InvalidOutputFormat;
            }

            break;
        }
    }

    emit finished(status);
}

void BlackBerryCertificate::processError()
{
    m_process->disconnect();

    emit finished(Error);
}

QString BlackBerryCertificate::command() const
{
    QString command = BlackBerryConfiguration::instance()
        .qnxEnv().value(QLatin1String("QNX_HOST"))
        + QLatin1String("/usr/bin/blackberry-keytool");

    if (Utils::HostOsInfo::isWindowsHost())
        command += QLatin1String(".bat");

    return command;
}

} // namespace Internal
} // namespace Qnx
