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

QString MercurialSettings::binary()
{
    return bin;
}

QString MercurialSettings::application()
{
    return app;
}

QStringList MercurialSettings::standardArguments()
{
    return standardArgs;
}

QString MercurialSettings::userName()
{
    return user;
}

QString MercurialSettings::email()
{
    return mail;
}

int MercurialSettings::logCount()
{
    return m_logCount;
}

int MercurialSettings::timeout()
{
    //return timeout is in Ms
    return m_timeout * 1000;
}

int MercurialSettings::timeoutSeconds()
{
    //return timeout in seconds (as the user specifies on the options page
    return m_timeout;
}

bool MercurialSettings::prompt()
{
    return m_prompt;
}

void MercurialSettings::writeSettings(const QString &application, const QString &userName,
                                      const QString &email, int logCount, int timeout, bool prompt)
{
    QSettings *settings = Core::ICore::instance()->settings();
    if (settings) {
        settings->beginGroup("Mercurial");
        settings->setValue(Constants::MERCURIALPATH, application);
        settings->setValue(Constants::MERCURIALUSERNAME, userName);
        settings->setValue(Constants::MERCURIALEMAIL, email);
        settings->setValue(Constants::MERCURIALLOGCOUNT, logCount);
        settings->setValue(Constants::MERCURIALTIMEOUT, timeout);
        settings->setValue(Constants::MERCURIALPROMPTSUBMIT, prompt);
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
        settings->beginGroup("Mercurial");
        app = settings->value(Constants::MERCURIALPATH, Constants::MERCURIALDEFAULT).toString();
        user = settings->value(Constants::MERCURIALUSERNAME, "").toString();
        mail = settings->value(Constants::MERCURIALEMAIL, "").toString();
        m_logCount = settings->value(Constants::MERCURIALLOGCOUNT, 0).toInt();
        m_timeout = settings->value(Constants::MERCURIALTIMEOUT, 30).toInt();
        m_prompt = settings->value(Constants::MERCURIALPROMPTSUBMIT, true).toBool();
        settings->endGroup();
    }

    setBinAndArgs();
}

void MercurialSettings::setBinAndArgs()
{
    standardArgs.clear();

#ifdef Q_OS_WIN
    bin = QLatin1String("cmd.exe");
    standardArgs << "/c" << app;
#else
    bin = app;
#endif
}
