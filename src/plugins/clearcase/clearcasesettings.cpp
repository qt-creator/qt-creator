// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clearcasesettings.h"

#include <utils/environment.h>

#include <QSettings>

namespace ClearCase::Internal {

const char groupC[] = "ClearCase";
const char commandKeyC[] = "Command";

const char historyCountKeyC[] = "HistoryCount";
const char timeOutKeyC[] = "TimeOut";
const char autoCheckOutKeyC[] = "AutoCheckOut";
const char noCommentKeyC[] = "NoComment";
const char keepFileUndoCheckoutKeyC[] = "KeepFileUnDoCheckout";
const char diffTypeKeyC[] = "DiffType";
const char diffArgsKeyC[] = "DiffArgs";
const char autoAssignActivityKeyC[] = "AutoAssignActivityName";
const char disableIndexerKeyC[] = "DisableIndexer";
const char totalFilesKeyC[] = "TotalFiles";
const char indexOnlyVOBsC[] = "IndexOnlyVOBs";

const char defaultDiffArgs[] = "-ubp";

enum { defaultTimeOutS = 30, defaultHistoryCount = 50 };

static QString defaultCommand()
{
    return QLatin1String("cleartool" QTC_HOST_EXE_SUFFIX);
}

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
    ccBinaryPath = Utils::Environment::systemEnvironment().searchInPath(ccCommand);
    timeOutS = settings->value(QLatin1String(timeOutKeyC), defaultTimeOutS).toInt();
    autoCheckOut = settings->value(QLatin1String(autoCheckOutKeyC), false).toBool();
    noComment = settings->value(QLatin1String(noCommentKeyC), false).toBool();
    keepFileUndoCheckout = settings->value(QLatin1String(keepFileUndoCheckoutKeyC), true).toBool();
    const QString sDiffType = settings->value(QLatin1String(diffTypeKeyC),
                                              QLatin1String("Graphical")).toString();
    switch (sDiffType[0].toUpper().toLatin1()) {
        case 'G': diffType = GraphicalDiff; break;
        case 'E': diffType = ExternalDiff; break;
    }

    diffArgs = settings->value(QLatin1String(diffArgsKeyC), QLatin1String(defaultDiffArgs)).toString();
    autoAssignActivityName = settings->value(QLatin1String(autoAssignActivityKeyC), true).toBool();
    historyCount = settings->value(QLatin1String(historyCountKeyC), int(defaultHistoryCount)).toInt();
    disableIndexer = settings->value(QLatin1String(disableIndexerKeyC), false).toBool();
    indexOnlyVOBs = settings->value(QLatin1String(indexOnlyVOBsC), QString()).toString();
    extDiffAvailable = !Utils::Environment::systemEnvironment().searchInPath(QLatin1String("diff")).isEmpty();
    settings->beginGroup(QLatin1String(totalFilesKeyC));
    const QStringList views = settings->childKeys();
    for (const QString &view : views)
        totalFiles[view] = settings->value(view).toInt();
    settings->endGroup();
    settings->endGroup();
}

void ClearCaseSettings::toSettings(QSettings *settings) const
{
    using FilesConstIt = QHash<QString, int>::ConstIterator;

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
        && disableIndexer         == s.disableIndexer
        && indexOnlyVOBs          == s.indexOnlyVOBs
        && totalFiles             == s.totalFiles;
}

} // ClearCase::Internal
