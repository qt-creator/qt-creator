/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberrycertificate.h"
#include "blackberryapilevelconfiguration.h"
#include "blackberryconfigurationmanager.h"
#include "blackberryndkprocess.h"

#include <utils/environment.h>
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
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    m_process->setEnvironment(Utils::EnvironmentItem::toStringList(
             BlackBerryConfigurationManager::instance()->defaultConfigurationEnv()));
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

        if (chunk.contains(QLatin1String("invalid password"))) {
            status = WrongPassword;
            break;
        } else if (chunk.contains(QLatin1String("must be at least 6 characters"))) {
            status = PasswordTooSmall;
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
    return BlackBerryNdkProcess::resolveNdkToolPath(QLatin1String("blackberry-keytool"));
}

} // namespace Internal
} // namespace Qnx
