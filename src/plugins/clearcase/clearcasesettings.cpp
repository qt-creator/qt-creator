/****************************************************************************
**
** Copyright (C) 2016 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "clearcasesettings.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QSettings>

static const char groupC[] = "ClearCase";
static const char commandKeyC[] = "Command";

static const char historyCountKeyC[] = "HistoryCount";
static const char timeOutKeyC[] = "TimeOut";
static const char autoCheckOutKeyC[] = "AutoCheckOut";
static const char noCommentKeyC[] = "NoComment";
static const char keepFileUndoCheckoutKeyC[] = "KeepFileUnDoCheckout";
static const char diffTypeKeyC[] = "DiffType";
static const char diffArgsKeyC[] = "DiffArgs";
static const char autoAssignActivityKeyC[] = "AutoAssignActivityName";
static const char promptToCheckInKeyC[] = "PromptToCheckIn";
static const char disableIndexerKeyC[] = "DisableIndexer";
static const char totalFilesKeyC[] = "TotalFiles";
static const char indexOnlyVOBsC[] = "IndexOnlyVOBs";

static const char defaultDiffArgs[] = "-ubp";

enum { defaultTimeOutS = 30, defaultHistoryCount = 50 };

static QString defaultCommand()
{
    return QLatin1String("cleartool" QTC_HOST_EXE_SUFFIX);
}

using namespace ClearCase::Internal;

ClearCaseSettings::ClearCaseSettings() :
    ccCommand(defaultCommand()),
    diffArgs(QLatin1String(defaultDiffArgs)),
    historyCount(defaultHistoryCount),
    timeOutS(defaultTimeOutS)
{ }

void ClearCaseSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(groupC));
    ccCommand = settings->value(QLatin1String(commandKeyC), defaultCommand()).toString();
    ccBinaryPath = Utils::Environment::systemEnvironment().searchInPath(ccCommand).toString();
    timeOutS = settings->value(QLatin1String(timeOutKeyC), defaultTimeOutS).toInt();
    autoCheckOut = settings->value(QLatin1String(autoCheckOutKeyC), false).toBool();
    noComment = settings->value(QLatin1String(noCommentKeyC), false).toBool();
    keepFileUndoCheckout = settings->value(QLatin1String(keepFileUndoCheckoutKeyC), true).toBool();
    QString sDiffType = settings->value(QLatin1String(diffTypeKeyC), QLatin1String("Graphical")).toString();
    switch (sDiffType[0].toUpper().toLatin1()) {
        case 'G': diffType = GraphicalDiff; break;
        case 'E': diffType = ExternalDiff; break;
    }

    diffArgs = settings->value(QLatin1String(diffArgsKeyC), QLatin1String(defaultDiffArgs)).toString();
    autoAssignActivityName = settings->value(QLatin1String(autoAssignActivityKeyC), true).toBool();
    historyCount = settings->value(QLatin1String(historyCountKeyC), int(defaultHistoryCount)).toInt();
    promptToCheckIn = settings->value(QLatin1String(promptToCheckInKeyC), false).toBool();
    disableIndexer = settings->value(QLatin1String(disableIndexerKeyC), false).toBool();
    indexOnlyVOBs = settings->value(QLatin1String(indexOnlyVOBsC), QString()).toString();
    extDiffAvailable = !Utils::Environment::systemEnvironment().searchInPath(QLatin1String("diff")).isEmpty();
    settings->beginGroup(QLatin1String(totalFilesKeyC));
    foreach (const QString &view, settings->childKeys())
        totalFiles[view] = settings->value(view).toInt();
    settings->endGroup();
    settings->endGroup();
}

void ClearCaseSettings::toSettings(QSettings *settings) const
{
    typedef QHash<QString, int>::ConstIterator FilesConstIt;

    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(QLatin1String(commandKeyC), ccCommand);
    settings->setValue(QLatin1String(autoCheckOutKeyC), autoCheckOut);
    settings->setValue(QLatin1String(noCommentKeyC), noComment);
    settings->setValue(QLatin1String(keepFileUndoCheckoutKeyC), keepFileUndoCheckout);
    settings->setValue(QLatin1String(timeOutKeyC), timeOutS);
    QString sDiffType;
    switch (diffType) {
        case ExternalDiff:  sDiffType = QLatin1String("External");  break;
        default:            sDiffType = QLatin1String("Graphical"); break;
    }

    settings->setValue(QLatin1String(diffArgsKeyC), diffArgs);
    settings->setValue(QLatin1String(diffTypeKeyC), sDiffType);
    settings->setValue(QLatin1String(autoAssignActivityKeyC), autoAssignActivityName);
    settings->setValue(QLatin1String(historyCountKeyC), historyCount);
    settings->setValue(QLatin1String(promptToCheckInKeyC), promptToCheckIn);
    settings->setValue(QLatin1String(disableIndexerKeyC), disableIndexer);
    settings->setValue(QLatin1String(indexOnlyVOBsC), indexOnlyVOBs);
    settings->beginGroup(QLatin1String(totalFilesKeyC));
    const FilesConstIt fcend = totalFiles.constEnd();
    for (FilesConstIt it = totalFiles.constBegin(); it != fcend; ++it)
        settings->setValue(it.key(), it.value());
    settings->endGroup();
    settings->endGroup();
}

bool ClearCaseSettings::equals(const ClearCaseSettings &s) const
{
    return ccCommand              == s.ccCommand
        && historyCount           == s.historyCount
        && timeOutS               == s.timeOutS
        && autoCheckOut           == s.autoCheckOut
        && noComment              == s.noComment
        && keepFileUndoCheckout   == s.keepFileUndoCheckout
        && diffType               == s.diffType
        && diffArgs               == s.diffArgs
        && autoAssignActivityName == s.autoAssignActivityName
        && promptToCheckIn        == s.promptToCheckIn
        && disableIndexer         == s.disableIndexer
        && indexOnlyVOBs          == s.indexOnlyVOBs
        && totalFiles             == s.totalFiles;
}
