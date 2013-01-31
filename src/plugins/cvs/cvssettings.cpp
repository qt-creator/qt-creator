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

#include "cvssettings.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QSettings>
#include <QTextStream>

static const char groupC[] = "CVS";
static const char commandKeyC[] = "Command";
static const char rootC[] = "Root";
static const char promptToSubmitKeyC[] = "PromptForSubmit";
static const char diffOptionsKeyC[] = "DiffOptions";
static const char describeByCommitIdKeyC[] = "DescribeByCommitId";
static const char defaultDiffOptions[] = "-du";
static const char timeOutKeyC[] = "TimeOut";

enum { defaultTimeOutS = 30 };

static QString defaultCommand()
{
    return QLatin1String("cvs" QTC_HOST_EXE_SUFFIX);
}

namespace Cvs {
namespace Internal {

CvsSettings::CvsSettings() :
    cvsCommand(defaultCommand()),
    cvsDiffOptions(QLatin1String(defaultDiffOptions)),
    timeOutS(defaultTimeOutS),
    promptToSubmit(true),
    describeByCommitId(true)
{
}

void CvsSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(groupC));
    cvsCommand = settings->value(QLatin1String(commandKeyC), defaultCommand()).toString();
    cvsBinaryPath = Utils::Environment::systemEnvironment().searchInPath(cvsCommand);
    promptToSubmit = settings->value(QLatin1String(promptToSubmitKeyC), true).toBool();
    cvsRoot = settings->value(QLatin1String(rootC), QString()).toString();
    cvsDiffOptions = settings->value(QLatin1String(diffOptionsKeyC), QLatin1String(defaultDiffOptions)).toString();
    describeByCommitId = settings->value(QLatin1String(describeByCommitIdKeyC), true).toBool();
    timeOutS = settings->value(QLatin1String(timeOutKeyC), defaultTimeOutS).toInt();
    settings->endGroup();
}

void CvsSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(QLatin1String(commandKeyC), cvsCommand);
    settings->setValue(QLatin1String(promptToSubmitKeyC), promptToSubmit);
    settings->setValue(QLatin1String(rootC), cvsRoot);
    settings->setValue(QLatin1String(diffOptionsKeyC), cvsDiffOptions);
    settings->setValue(QLatin1String(timeOutKeyC), timeOutS);
    settings->setValue(QLatin1String(describeByCommitIdKeyC), describeByCommitId);
    settings->endGroup();
}

bool CvsSettings::equals(const CvsSettings &s) const
{
    return promptToSubmit     == s.promptToSubmit
        && describeByCommitId == s.describeByCommitId
        && cvsCommand         == s.cvsCommand
        && cvsRoot            == s.cvsRoot
        && timeOutS           == s.timeOutS
        && cvsDiffOptions     == s.cvsDiffOptions;
}

QStringList CvsSettings::addOptions(const QStringList &args) const
{
    if (cvsRoot.isEmpty())
        return args;

    QStringList rc;
    rc.push_back(QLatin1String("-d"));
    rc.push_back(cvsRoot);
    rc.append(args);
    return rc;
}

} // namespace Internal
} // namespace Cvs
