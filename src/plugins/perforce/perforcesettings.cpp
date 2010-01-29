/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "perforcesettings.h"

#include <qtconcurrent/QtConcurrentTools>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>

enum { debug = 0 };

static const char *groupC = "Perforce";
static const char *commandKeyC = "Command";
static const char *defaultKeyC = "Default";
static const char *portKeyC = "Port";
static const char *clientKeyC = "Client";
static const char *userKeyC = "User";
static const char *promptToSubmitKeyC = "PromptForSubmit";

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

Settings::Settings() :
    defaultEnv(true),
    promptToSubmit(true)
{
}

bool Settings::equals(const Settings &rhs) const
{
    return defaultEnv == rhs.defaultEnv
            && p4Command == rhs.p4Command && p4Port == rhs.p4Port
            && p4Client == rhs.p4Client && p4User == rhs.p4User
            && promptToSubmit == rhs.promptToSubmit;
};

QStringList Settings::basicP4Args() const
{
    if (defaultEnv)
        return QStringList();
    QStringList lst;
    if (!p4Client.isEmpty())
        lst << QLatin1String("-c") << p4Client;
    if (!p4Port.isEmpty())
        lst << QLatin1String("-p") << p4Port;
    if (!p4User.isEmpty())
        lst << QLatin1String("-u") << p4User;
    return lst;
}

bool Settings::check(QString *errorMessage) const
{
    return doCheck(p4Command, basicP4Args(), errorMessage);
}

// Check on a p4 view by grepping "view -o" for mapped files
bool Settings::doCheck(const QString &binary, const QStringList &basicArgs, QString *errorMessage)
{
    errorMessage->clear();
    if (binary.isEmpty()) {
        *errorMessage = QCoreApplication::translate("Perforce::Internal",  "No executable specified");
        return false;
    }
    // List the client state and check for mapped files
    QProcess p4;
    QStringList args = basicArgs;
    args << QLatin1String("client") << QLatin1String("-o");
    if (debug)
        qDebug() << binary << args;
    p4.start(binary, args);
    if (!p4.waitForStarted()) {
        *errorMessage = QCoreApplication::translate("Perforce::Internal", "Unable to launch \"%1\": %2").arg(binary, p4.errorString());
        return false;
    }
    p4.closeWriteChannel();
    const int timeOutMS = 5000;
    if (!p4.waitForFinished(timeOutMS)) {
        p4.kill();
        p4.waitForFinished();
        *errorMessage = QCoreApplication::translate("Perforce::Internal", "\"%1\" timed out after %2ms.").arg(binary).arg(timeOutMS);
        return false;
    }
    if (p4.exitStatus() != QProcess::NormalExit) {
        *errorMessage = QCoreApplication::translate("Perforce::Internal", "\"%1\" crashed.").arg(binary);
        return false;
    }
    const QString stdErr = QString::fromLocal8Bit(p4.readAllStandardError());
    if (p4.exitCode()) {
        *errorMessage = QCoreApplication::translate("Perforce::Internal", "\"%1\" terminated with exit code %2: %3").
                        arg(binary).arg(p4.exitCode()).arg(stdErr);
        return false;

    }
    // List the client state and check for "View"
    const QString response = QString::fromLocal8Bit(p4.readAllStandardOutput());
    if (!response.contains(QLatin1String("View:")) && !response.contains(QLatin1String("//depot/"))) {
        *errorMessage = QCoreApplication::translate("Perforce::Internal", "The client does not seem to contain any mapped files.");
        return false;
    }
    return true;
}

// --------------------PerforceSettings
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
    const QString executable = m_settings.p4Command;
    const QStringList arguments = basicP4Args();
    m_mutex.unlock();

    QString errorString;
    const bool isValid = Settings::doCheck(executable, arguments, &errorString);
    if (debug)
        qDebug() << isValid << errorString;

    m_mutex.lock();
    if (executable == m_settings.p4Command && arguments == basicP4Args()) { // Check that those settings weren't changed in between
        m_errorString = errorString;
        m_valid = isValid;
    }
    m_mutex.unlock();
    fi.reportFinished();
}

void PerforceSettings::fromSettings(QSettings *settings)
{
    m_mutex.lock();
    settings->beginGroup(QLatin1String(groupC));
    m_settings.p4Command = settings->value(QLatin1String(commandKeyC), defaultCommand()).toString();
    m_settings.defaultEnv = settings->value(QLatin1String(defaultKeyC), true).toBool();
    m_settings.p4Port = settings->value(QLatin1String(portKeyC), QString()).toString();
    m_settings.p4Client = settings->value(QLatin1String(clientKeyC), QString()).toString();
    m_settings.p4User = settings->value(QLatin1String(userKeyC), QString()).toString();
    m_settings.promptToSubmit = settings->value(QLatin1String(promptToSubmitKeyC), true).toBool();
    settings->endGroup();
    m_mutex.unlock();

    m_future = QtConcurrent::run(&PerforceSettings::run, this);
}

void PerforceSettings::toSettings(QSettings *settings) const
{
    m_mutex.lock();
    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(QLatin1String(commandKeyC), m_settings.p4Command);
    settings->setValue(QLatin1String(defaultKeyC), m_settings.defaultEnv);
    settings->setValue(QLatin1String(portKeyC), m_settings.p4Port);
    settings->setValue(QLatin1String(clientKeyC), m_settings.p4Client);
    settings->setValue(QLatin1String(userKeyC), m_settings.p4User);
    settings->setValue(QLatin1String(promptToSubmitKeyC), m_settings.promptToSubmit);
    settings->endGroup();
    m_mutex.unlock();
}

void PerforceSettings::setSettings(const Settings &newSettings)
{
    if (newSettings != m_settings) {
        // trigger check
        m_settings = newSettings;
        m_mutex.lock();
        m_valid = false;
        m_mutex.unlock();
        m_future = QtConcurrent::run(&PerforceSettings::run, this);
    }
}

Settings PerforceSettings::settings() const
{
    return m_settings;
}

QString PerforceSettings::p4Command() const
{
    return m_settings.p4Command;
}

QString PerforceSettings::p4Port() const
{
    return m_settings.p4Port;
}

QString PerforceSettings::p4Client() const
{
    return m_settings.p4Client;
}

QString PerforceSettings::p4User() const
{
    return m_settings.p4User;
}

bool PerforceSettings::defaultEnv() const
{
    return m_settings.defaultEnv;
}

bool PerforceSettings::promptToSubmit() const
{
    return m_settings.promptToSubmit;
}

void PerforceSettings::setPromptToSubmit(bool p)
{
    m_settings.promptToSubmit = p;
}

QString PerforceSettings::errorString() const
{
    m_mutex.lock();
    const QString rc = m_errorString;
    m_mutex.unlock();
    return rc;
}

QStringList PerforceSettings::basicP4Args() const
{
    return m_settings.basicP4Args();
}

} // Internal
} // Perforce
