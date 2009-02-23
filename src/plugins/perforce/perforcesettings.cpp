/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "perforcesettings.h"

#include <qtconcurrent/QtConcurrentTools>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtCore/QProcess>

static const char *groupC = "Perforce";
static const char *commandKeyC = "Command";
static const char *defaultKeyC = "Default";
static const char *portKeyC = "Port";
static const char *clientKeyC = "Client";
static const char *userKeyC = "User";

static QString defaultCommand()
{
    QString rc;
    rc = QLatin1String("p4");
#if defined(Q_OS_WIN32)
    rc.append(QLatin1String(".exe"));
#endif
    return rc;
}

namespace Perforce {
namespace Internal {

PerforceSettings::PerforceSettings()
    : m_valid(false)
{
    // We do all the initialization in fromSettings
}

PerforceSettings::~PerforceSettings()
{
    // ensure that we are not still running
    m_future.waitForFinished();
}

bool PerforceSettings::isValid() const
{
    m_future.waitForFinished();
    m_mutex.lock();
    bool valid = m_valid;
    m_mutex.unlock();
    return valid;
}

void PerforceSettings::run(QFutureInterface<void> &fi)
{
    m_mutex.lock();
    QString executable = m_p4Command;
    QStringList arguments = basicP4Args();
    m_mutex.unlock();

    // TODO actually check
    bool valid = true;

    QProcess p4;
    p4.start(m_p4Command, QStringList() << "client"<<"-o");
    p4.waitForFinished(2000);
    if (p4.state() != QProcess::NotRunning) {
        p4.kill();
        p4.waitForFinished();
        valid = false;
    } else {
        QString response = p4.readAllStandardOutput();
        if (!response.contains("View:"))
            valid = false;
    }

    m_mutex.lock();
    if (executable == m_p4Command && arguments == basicP4Args()) // Check that those settings weren't changed in between
        m_valid = valid;
    m_mutex.unlock();
    fi.reportFinished();
}

void PerforceSettings::fromSettings(QSettings *settings)
{
    m_mutex.lock();
    settings->beginGroup(QLatin1String(groupC));
    m_p4Command = settings->value(QLatin1String(commandKeyC), defaultCommand()).toString();
    m_defaultEnv = settings->value(QLatin1String(defaultKeyC), true).toBool();
    m_p4Port = settings->value(QLatin1String(portKeyC), QString()).toString();
    m_p4Client = settings->value(QLatin1String(clientKeyC), QString()).toString();
    m_p4User = settings->value(QLatin1String(userKeyC), QString()).toString();
    settings->endGroup();
    m_mutex.unlock();

    m_future = QtConcurrent::run(&PerforceSettings::run, this);
}

void PerforceSettings::toSettings(QSettings *settings) const
{
    m_mutex.lock();
    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(commandKeyC, m_p4Command);
    settings->setValue(defaultKeyC, m_defaultEnv);
    settings->setValue(portKeyC, m_p4Port);
    settings->setValue(clientKeyC, m_p4Client);
    settings->setValue(userKeyC, m_p4User);
    settings->endGroup();
    m_mutex.unlock();
}

void PerforceSettings::setSettings(const QString &p4Command, const QString &p4Port, const QString &p4Client, const QString p4User, bool defaultEnv)
{
    m_mutex.lock();
    m_p4Command = p4Command;
    m_p4Port = p4Port;
    m_p4Client = p4Client;
    m_p4User = p4User;
    m_defaultEnv = defaultEnv;
    m_valid = false;
    m_mutex.unlock();
    m_future = QtConcurrent::run(&PerforceSettings::run, this);
}

QString PerforceSettings::p4Command() const
{
    return m_p4Command;
}

QString PerforceSettings::p4Port() const
{
    return m_p4Port;
}

QString PerforceSettings::p4Client() const
{
    return m_p4Client;
}

QString PerforceSettings::p4User() const
{
    return m_p4User;
}

bool PerforceSettings::defaultEnv() const
{
    return m_defaultEnv;
}

QStringList PerforceSettings::basicP4Args() const
{
    QStringList lst;
    if (!m_defaultEnv) {
        lst << QLatin1String("-c") << m_p4Client;
        lst << QLatin1String("-p") << m_p4Port;
        lst << QLatin1String("-u") << m_p4User;
    }
    return lst;
}


} // Internal
} // Perforce
