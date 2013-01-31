/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "subversionsettings.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QSettings>

static const char groupC[] = "Subversion";
static const char commandKeyC[] = "Command";
static const char userKeyC[] = "User";
static const char passwordKeyC[] = "Password";
static const char authenticationKeyC[] = "Authentication";

static const char promptToSubmitKeyC[] = "PromptForSubmit";
static const char timeOutKeyC[] = "TimeOut";
static const char spaceIgnorantAnnotationKeyC[] = "SpaceIgnorantAnnotation";
static const char logCountKeyC[] = "LogCount";

enum { defaultTimeOutS = 30, defaultLogCount = 1000 };

static QString defaultCommand()
{
    return QLatin1String("svn" QTC_HOST_EXE_SUFFIX);
}

using namespace Subversion::Internal;

SubversionSettings::SubversionSettings() :
    svnCommand(defaultCommand()),
    useAuthentication(false),
    logCount(defaultLogCount),
    timeOutS(defaultTimeOutS),
    promptToSubmit(true),
    spaceIgnorantAnnotation(true)
{
}

void SubversionSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(groupC));
    svnCommand = settings->value(QLatin1String(commandKeyC), defaultCommand()).toString();
    svnBinaryPath = Utils::Environment::systemEnvironment().searchInPath(svnCommand);
    useAuthentication = settings->value(QLatin1String(authenticationKeyC), QVariant(false)).toBool();
    user = settings->value(QLatin1String(userKeyC), QString()).toString();
    password =  settings->value(QLatin1String(passwordKeyC), QString()).toString();
    timeOutS = settings->value(QLatin1String(timeOutKeyC), defaultTimeOutS).toInt();
    promptToSubmit = settings->value(QLatin1String(promptToSubmitKeyC), true).toBool();
    spaceIgnorantAnnotation = settings->value(QLatin1String(spaceIgnorantAnnotationKeyC), true).toBool();
    logCount = settings->value(QLatin1String(logCountKeyC), int(defaultLogCount)).toInt();
    settings->endGroup();
}

void SubversionSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(QLatin1String(commandKeyC), svnCommand);
    settings->setValue(QLatin1String(authenticationKeyC), QVariant(useAuthentication));
    settings->setValue(QLatin1String(userKeyC), user);
    settings->setValue(QLatin1String(passwordKeyC), password);
    settings->setValue(QLatin1String(promptToSubmitKeyC), promptToSubmit);
    settings->setValue(QLatin1String(timeOutKeyC), timeOutS);
    settings->setValue(QLatin1String(spaceIgnorantAnnotationKeyC), spaceIgnorantAnnotation);
    settings->setValue(QLatin1String(logCountKeyC), logCount);
    settings->endGroup();
}

bool SubversionSettings::equals(const SubversionSettings &s) const
{
    return svnCommand        == s.svnCommand
        && useAuthentication == s.useAuthentication
        && user              == s.user
        && password          == s.password
        && logCount          == s.logCount
        && timeOutS          == s.timeOutS
        && promptToSubmit    == s.promptToSubmit
        && spaceIgnorantAnnotation == s.spaceIgnorantAnnotation;
}
