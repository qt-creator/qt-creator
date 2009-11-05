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

#include <coreplugin/icore.h>
#include <QtCore/QSettings>


using namespace Mercurial::Internal;

MercurialSettings::MercurialSettings()
{
    readSettings();
}

QString MercurialSettings::binary() const
{
    return bin;
}

QString MercurialSettings::application() const
{
    return app;
}

QStringList MercurialSettings::standardArguments() const
{
    return standardArgs;
}

QString MercurialSettings::userName() const
{
    return user;
}

QString MercurialSettings::email() const
{
    return mail;
}

int MercurialSettings::logCount() const
{
    return m_logCount;
}

int MercurialSettings::timeout() const
{
    //return timeout is in Ms
    return m_timeout * 1000;
}

int MercurialSettings::timeoutSeconds() const
{
    //return timeout in seconds (as the user specifies on the options page
    return m_timeout;
}

bool MercurialSettings::prompt() const
{
    return m_prompt;
}

void MercurialSettings::writeSettings(const QString &application, const QString &userName,
                                      const QString &email, int logCount, int timeout, bool prompt)
{
    QSettings *settings = Core::ICore::instance()->settings();
    if (settings) {
        settings->beginGroup(QLatin1String("Mercurial"));
        settings->setValue(QLatin1String(Constants::MERCURIALPATH), application);
        settings->setValue(QLatin1String(Constants::MERCURIALUSERNAME), userName);
        settings->setValue(QLatin1String(Constants::MERCURIALEMAIL), email);
        settings->setValue(QLatin1String(Constants::MERCURIALLOGCOUNT), logCount);
        settings->setValue(QLatin1String(Constants::MERCURIALTIMEOUT), timeout);
        settings->setValue(QLatin1String(Constants::MERCURIALPROMPTSUBMIT), prompt);
        settings->endGroup();
    }

    app = application;
    user = userName;
    mail = email;
    m_logCount = logCount;
    m_timeout = timeout;
    m_prompt = prompt;
    setBinAndArgs();
}

void MercurialSettings::readSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();
    if (settings) {
        settings->beginGroup(QLatin1String("Mercurial"));
        app = settings->value(QLatin1String(Constants::MERCURIALPATH),
                              QLatin1String(Constants::MERCURIALDEFAULT)).toString();
        user = settings->value(QLatin1String(Constants::MERCURIALUSERNAME), QString()).toString();
        mail = settings->value(QLatin1String(Constants::MERCURIALEMAIL), QString()).toString();
        m_logCount = settings->value(QLatin1String(Constants::MERCURIALLOGCOUNT), 0).toInt();
        m_timeout = settings->value(QLatin1String(Constants::MERCURIALTIMEOUT), 30).toInt();
        m_prompt = settings->value(QLatin1String(Constants::MERCURIALPROMPTSUBMIT), true).toBool();
        settings->endGroup();
    }

    setBinAndArgs();
}

void MercurialSettings::setBinAndArgs()
{
    standardArgs.clear();
    bin = app;
}
