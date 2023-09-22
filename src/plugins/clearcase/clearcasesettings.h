// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/storekey.h>

#include <QHash>

namespace Utils { class QtcSettings; }

namespace ClearCase::Internal {

enum DiffType
{
    GraphicalDiff,
    ExternalDiff
};

class ClearCaseSettings
{
public:
    ClearCaseSettings();

    void fromSettings(Utils::QtcSettings *);
    void toSettings(Utils::QtcSettings *) const;

    bool equals(const ClearCaseSettings &s) const;

    friend bool operator==(const ClearCaseSettings &p1, const ClearCaseSettings &p2)
    { return p1.equals(p2); }
    friend bool operator!=(const ClearCaseSettings &p1, const ClearCaseSettings &p2)
    { return !p1.equals(p2); }

    QString ccCommand;
    Utils::FilePath ccBinaryPath;
    DiffType diffType = GraphicalDiff;
    QString diffArgs;
    QString indexOnlyVOBs;
    QHash<Utils::Key, int> totalFiles;
    bool autoAssignActivityName = true;
    bool autoCheckOut = true;
    bool noComment = false;
    bool keepFileUndoCheckout = true;
    bool disableIndexer = false;
    bool extDiffAvailable = false;
    int historyCount;
    int timeOutS;
};

} // ClearCase::Internal
