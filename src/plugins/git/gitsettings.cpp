/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "gitsettings.h"

#include <QtCore/QSettings>
#include <QtCore/QTextStream>

static const char *groupC = "Git";
static const char *sysEnvKeyC = "SysEnv";
static const char *pathKeyC = "Path";
static const char *logCountKeyC = "LogCount";
static const char *timeoutKeyC = "TimeOut";

enum { defaultLogCount =  10 , defaultTimeOut = 30};

namespace Git {
namespace Internal {

GitSettings::GitSettings() :
    adoptPath(false),
    logCount(defaultLogCount),
    timeout(defaultTimeOut)
{
}

void GitSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(groupC));
    adoptPath = settings->value(QLatin1String(sysEnvKeyC), false).toBool();
    path = settings->value(QLatin1String(pathKeyC), QString()).toString();
    logCount = settings->value(QLatin1String(logCountKeyC), defaultLogCount).toInt();
    timeout = settings->value(QLatin1String(timeoutKeyC), defaultTimeOut).toInt();
    settings->endGroup();
}

void GitSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(QLatin1String(sysEnvKeyC), adoptPath);
    settings->setValue(QLatin1String(pathKeyC), path);
    settings->setValue(QLatin1String(logCountKeyC), logCount);
    settings->setValue(QLatin1String(timeoutKeyC), timeout);
    settings->endGroup();
}

bool GitSettings::equals(const GitSettings &s) const
{
    return adoptPath == s.adoptPath && path == s.path && logCount == s.logCount && timeout == s.timeout;
}

}
}
