// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clearcasesettings.h"

#include <utils/environment.h>
#include <utils/qtcsettings.h>

using namespace Utils;

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

void ClearCaseSettings::fromSettings(QtcSettings *settings)
{
    settings->beginGroup(groupC);
    ccCommand = settings->value(commandKeyC, defaultCommand()).toString();
    ccBinaryPath = Utils::Environment::systemEnvironment().searchInPath(ccCommand);
    timeOutS = settings->value(timeOutKeyC, defaultTimeOutS).toInt();
    autoCheckOut = settings->value(autoCheckOutKeyC, false).toBool();
    noComment = settings->value(noCommentKeyC, false).toBool();
    keepFileUndoCheckout = settings->value(keepFileUndoCheckoutKeyC, true).toBool();
    const QString sDiffType = settings->value(diffTypeKeyC,
                                              QLatin1String("Graphical")).toString();
    switch (sDiffType[0].toUpper().toLatin1()) {
        case 'G': diffType = GraphicalDiff; break;
        case 'E': diffType = ExternalDiff; break;
    }

    diffArgs = settings->value(diffArgsKeyC, QLatin1String(defaultDiffArgs)).toString();
    autoAssignActivityName = settings->value(autoAssignActivityKeyC, true).toBool();
    historyCount = settings->value(historyCountKeyC, int(defaultHistoryCount)).toInt();
    disableIndexer = settings->value(disableIndexerKeyC, false).toBool();
    indexOnlyVOBs = settings->value(indexOnlyVOBsC, QString()).toString();
    extDiffAvailable = !Utils::Environment::systemEnvironment().searchInPath(QLatin1String("diff")).isEmpty();
    settings->beginGroup(totalFilesKeyC);
    const KeyList views = settings->childKeys();
    for (const Key &view : views)
        totalFiles[view] = settings->value(view).toInt();
    settings->endGroup();
    settings->endGroup();
}

void ClearCaseSettings::toSettings(QtcSettings *settings) const
{
    using FilesConstIt = QHash<Key, int>::ConstIterator;

    settings->beginGroup(groupC);
    settings->setValue(commandKeyC, ccCommand);
    settings->setValue(autoCheckOutKeyC, autoCheckOut);
    settings->setValue(noCommentKeyC, noComment);
    settings->setValue(keepFileUndoCheckoutKeyC, keepFileUndoCheckout);
    settings->setValue(timeOutKeyC, timeOutS);
    QString sDiffType;
    switch (diffType) {
        case ExternalDiff:  sDiffType = QLatin1String("External");  break;
        default:            sDiffType = QLatin1String("Graphical"); break;
    }

    settings->setValue(diffArgsKeyC, diffArgs);
    settings->setValue(diffTypeKeyC, sDiffType);
    settings->setValue(autoAssignActivityKeyC, autoAssignActivityName);
    settings->setValue(historyCountKeyC, historyCount);
    settings->setValue(disableIndexerKeyC, disableIndexer);
    settings->setValue(indexOnlyVOBsC, indexOnlyVOBs);
    settings->beginGroup(totalFilesKeyC);
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
