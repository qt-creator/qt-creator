/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include "perforcesettings.h"

#include <QtCore/QSettings>

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

PerforceSettings::PerforceSettings() :
    p4Command(defaultCommand()),
    defaultEnv(true)
{
}

void PerforceSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(groupC));
    p4Command = settings->value(QLatin1String(commandKeyC), defaultCommand()).toString();
    defaultEnv = settings->value(QLatin1String(defaultKeyC), true).toBool();
    p4Port = settings->value(QLatin1String(portKeyC), QString()).toString();
    p4Client = settings->value(QLatin1String(clientKeyC), QString()).toString();
    p4User = settings->value(QLatin1String(userKeyC), QString()).toString();
    settings->endGroup();

}

void PerforceSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(commandKeyC, p4Command);
    settings->setValue(defaultKeyC, defaultEnv);
    settings->setValue(portKeyC, p4Port);
    settings->setValue(clientKeyC, p4Client);
    settings->setValue(userKeyC, p4User);
    settings->endGroup();
}

bool PerforceSettings::equals(const PerforceSettings &s) const
{
    return p4Command  == s.p4Command && p4Port == s.p4Port
        && p4Client   == s.p4Client  &&  p4User == s.p4User
        && defaultEnv == s.defaultEnv;
}

}
}

