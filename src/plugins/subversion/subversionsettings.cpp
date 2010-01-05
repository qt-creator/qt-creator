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

#include "subversionsettings.h"

#include <QtCore/QSettings>
#include <QtCore/QTextStream>

static const char *groupC = "Subversion";
static const char *commandKeyC = "Command";
static const char *userKeyC = "User";
static const char *passwordKeyC = "Password";
static const char *authenticationKeyC = "Authentication";

static const char *userNameOptionC = "--username";
static const char *passwordOptionC = "--password";
static const char *promptToSubmitKeyC = "PromptForSubmit";
static const char *timeOutKeyC = "TimeOut";
static const char *spaceIgnorantAnnotationKeyC = "SpaceIgnorantAnnotation";

enum { defaultTimeOutS = 30 };

static QString defaultCommand()
{
    QString rc;
    rc = QLatin1String("svn");
#if defined(Q_OS_WIN32)
    rc.append(QLatin1String(".exe"));
#endif
    return rc;
}

using namespace Subversion::Internal;

SubversionSettings::SubversionSettings() :
    svnCommand(defaultCommand()),
    useAuthentication(false),
    timeOutS(defaultTimeOutS),
    promptToSubmit(true),
    spaceIgnorantAnnotation(true)
{
}

void SubversionSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(groupC));
    svnCommand = settings->value(QLatin1String(commandKeyC), defaultCommand()).toString();
    useAuthentication = settings->value(QLatin1String(authenticationKeyC), QVariant(false)).toBool();
    user = settings->value(QLatin1String(userKeyC), QString()).toString();
    password =  settings->value(QLatin1String(passwordKeyC), QString()).toString();
    timeOutS = settings->value(QLatin1String(timeOutKeyC), defaultTimeOutS).toInt();
    promptToSubmit = settings->value(QLatin1String(promptToSubmitKeyC), true).toBool();
    spaceIgnorantAnnotation = settings->value(QLatin1String(spaceIgnorantAnnotationKeyC), true).toBool();
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
    settings->endGroup();
}

bool SubversionSettings::equals(const SubversionSettings &s) const
{
    return svnCommand        == s.svnCommand
        && useAuthentication == s.useAuthentication
        && user              == s.user
        && password          == s.password
        && timeOutS          == s.timeOutS
        && promptToSubmit    == s.promptToSubmit
        && spaceIgnorantAnnotation == s.spaceIgnorantAnnotation;
}

QStringList SubversionSettings::addOptions(const QStringList &args) const
{
    if (!useAuthentication || user.isEmpty())
        return args;
    // SVN pre 1.5 does not accept "--userName" for "add", which is most likely
    // an oversight. As no password is needed for the option, generally omit it.
    if (!args.empty() && args.front() == QLatin1String("add"))
        return args;

    QStringList rc;
    rc.push_back(QLatin1String(userNameOptionC));
    rc.push_back(user);
    if (!password.isEmpty()) {
        rc.push_back(QLatin1String(passwordOptionC));
        rc.push_back(password);
    }
    rc.append(args);
    return rc;
}

// Format arguments for log windows hiding passwords, etc.
QString SubversionSettings::formatArguments(const QStringList &args)
{
    QString rc;
    QTextStream str(&rc);
    const int size = args.size();
    // Skip authentication options
    for (int i = 0; i < size; i++) {
        const QString &arg = args.at(i);
        if (i)
            str << ' ';
        str << arg;
        if (arg == QLatin1String(userNameOptionC) || arg == QLatin1String(passwordOptionC)) {
            str << " ********";
            i++;
        }
    }
    return rc;
}
