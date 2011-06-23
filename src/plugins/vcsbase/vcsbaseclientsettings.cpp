/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Brian McGillion & Hugues Delorme
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "vcsbaseclientsettings.h"

#include <QtCore/QSettings>

using namespace VCSBase;

enum { timeOutDefaultSeconds = 30 };

/*!
    \class VCSBase::VCSBaseClientSettings

    \brief Settings used in VCSBaseClient.

    \sa VCSBase::VCSBaseClient
*/

VCSBaseClientSettings::VCSBaseClientSettings() :
   m_binary(),
    m_logCount(0),
    m_prompt(true),
    m_timeoutSeconds(timeOutDefaultSeconds)
{
}

VCSBaseClientSettings::~VCSBaseClientSettings()
{ }

QString VCSBaseClientSettings::binary() const
{
    return m_binary;
}

void VCSBaseClientSettings::setBinary(const QString &b)
{
    m_binary = b;
}

QStringList VCSBaseClientSettings::standardArguments() const
{
    return m_standardArguments;
}

QString VCSBaseClientSettings::userName() const
{
    return m_user;
}

void VCSBaseClientSettings::setUserName(const QString &u)
{
    m_user = u;
}

QString VCSBaseClientSettings::email() const
{
    return m_mail;
}

void VCSBaseClientSettings::setEmail(const QString &m)
{
    m_mail = m;
}

int VCSBaseClientSettings::logCount() const
{
    return m_logCount;
}

void VCSBaseClientSettings::setLogCount(int l)
{
    m_logCount = l;
}

bool VCSBaseClientSettings::prompt() const
{
    return m_prompt;
}

void VCSBaseClientSettings::setPrompt(bool b)
{
    m_prompt = b;
}

int VCSBaseClientSettings::timeoutMilliSeconds() const
{
    //return timeout is in Ms
    return m_timeoutSeconds * 1000;
}

int VCSBaseClientSettings::timeoutSeconds() const
{
    //return timeout in seconds (as the user specifies on the options page
    return m_timeoutSeconds;
}

void VCSBaseClientSettings::setTimeoutSeconds(int s)
{
    m_timeoutSeconds = s;
}

QString VCSBaseClientSettings::settingsGroup() const
{
    return m_settingsGroup;
}

void VCSBaseClientSettings::setSettingsGroup(const QString &group)
{
    m_settingsGroup = group;
}

void VCSBaseClientSettings::writeSettings(QSettings *settings) const
{
    settings->beginGroup(settingsGroup());
    settings->setValue(QLatin1String("VCS_Path"), m_binary);
    settings->setValue(QLatin1String("VCS_Username"), m_user);
    settings->setValue(QLatin1String("VCS_Email"), m_mail);
    settings->setValue(QLatin1String("VCS_LogCount"), m_logCount);
    settings->setValue(QLatin1String("VCS_PromptOnSubmit"), m_prompt);
    settings->setValue(QLatin1String("VCS_Timeout"), m_timeoutSeconds);
    settings->endGroup();
}

void VCSBaseClientSettings::readSettings(const QSettings *settings)
{
    const QString keyRoot = settingsGroup() + QLatin1Char('/');
    m_binary = settings->value(keyRoot + QLatin1String("VCS_Path"), QString()).toString();
    m_user = settings->value(keyRoot + QLatin1String("VCS_Username"), QString()).toString();
    m_mail = settings->value(keyRoot + QLatin1String("VCS_Email"), QString()).toString();
    m_logCount = settings->value(keyRoot + QLatin1String("VCS_LogCount"), QString()).toInt();
    m_prompt = settings->value(keyRoot + QLatin1String("VCS_PromptOnSubmit"), QString()).toBool();
    m_timeoutSeconds = settings->value(keyRoot + QLatin1String("VCS_Timeout"), timeOutDefaultSeconds).toInt();
}

bool VCSBaseClientSettings::equals(const VCSBaseClientSettings &rhs) const
{
    return m_binary == rhs.m_binary && m_standardArguments == rhs.m_standardArguments
            && m_user == rhs.m_user && m_mail == rhs.m_mail
            && m_logCount == rhs.m_logCount && m_prompt == rhs.m_prompt
            && m_timeoutSeconds == rhs.m_timeoutSeconds;
}
