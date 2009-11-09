/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#include "mercurialsettings.h"
#include "constants.h"

#include <QtCore/QSettings>

using namespace Mercurial::Internal;

enum { timeOutDefaultSeconds = 30 };

MercurialSettings::MercurialSettings() :
   m_binary(QLatin1String(Constants::MERCURIALDEFAULT)),
   m_logCount(0),
   m_timeoutSeconds(timeOutDefaultSeconds),
   m_prompt(true)
{
}

QString MercurialSettings::binary() const
{
    return m_binary;
}

void MercurialSettings::setBinary(const QString &b)
{
    m_binary = b;
}

QStringList MercurialSettings::standardArguments() const
{
    return m_standardArguments;
}

QString MercurialSettings::userName() const
{
    return m_user;
}

void MercurialSettings::setUserName(const QString &u)
{
    m_user = u;
}

QString MercurialSettings::email() const
{
    return m_mail;
}

void MercurialSettings::setEmail(const QString &m)
{
    m_mail = m;
}

int MercurialSettings::logCount() const
{
    return m_logCount;
}

void MercurialSettings::setLogCount(int l)
{
    m_logCount = l;
}

int MercurialSettings::timeoutMilliSeconds() const
{
    //return timeout is in Ms
    return m_timeoutSeconds * 1000;
}

int MercurialSettings::timeoutSeconds() const
{
    //return timeout in seconds (as the user specifies on the options page
    return m_timeoutSeconds;
}

void MercurialSettings::setTimeoutSeconds(int s)
{
    m_timeoutSeconds = s;
}

bool MercurialSettings::prompt() const
{
    return m_prompt;
}

void MercurialSettings::setPrompt(bool b)
{
    m_prompt = b;
}

void MercurialSettings::writeSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String("Mercurial"));
    settings->setValue(QLatin1String(Constants::MERCURIALPATH), m_binary);
    settings->setValue(QLatin1String(Constants::MERCURIALUSERNAME), m_user);
    settings->setValue(QLatin1String(Constants::MERCURIALEMAIL), m_mail);
    settings->setValue(QLatin1String(Constants::MERCURIALLOGCOUNT), m_logCount);
    settings->setValue(QLatin1String(Constants::MERCURIALTIMEOUT), m_timeoutSeconds);
    settings->setValue(QLatin1String(Constants::MERCURIALPROMPTSUBMIT), m_prompt);
    settings->endGroup();
}

void MercurialSettings::readSettings(const QSettings *settings)
{
    const QString keyRoot = QLatin1String("Mercurial/");
    m_binary = settings->value(keyRoot + QLatin1String(Constants::MERCURIALPATH),
                               QLatin1String(Constants::MERCURIALDEFAULT)).toString();
    m_user = settings->value(keyRoot + QLatin1String(Constants::MERCURIALUSERNAME), QString()).toString();
    m_mail = settings->value(keyRoot + QLatin1String(Constants::MERCURIALEMAIL), QString()).toString();
    m_logCount = settings->value(keyRoot + QLatin1String(Constants::MERCURIALLOGCOUNT), 0).toInt();
    m_timeoutSeconds = settings->value(keyRoot + QLatin1String(Constants::MERCURIALTIMEOUT), timeOutDefaultSeconds).toInt();
    m_prompt = settings->value(keyRoot + QLatin1String(Constants::MERCURIALPROMPTSUBMIT), true).toBool();
}

bool MercurialSettings::equals(const MercurialSettings &rhs) const
{
 return m_binary == rhs.m_binary && m_standardArguments == rhs.m_standardArguments
         && m_user == rhs.m_user && m_mail == rhs.m_mail
         && m_logCount == rhs.m_logCount && m_timeoutSeconds == rhs.m_timeoutSeconds
         && m_prompt == rhs.m_prompt;
}
