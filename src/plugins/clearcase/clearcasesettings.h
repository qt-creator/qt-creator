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

#pragma once

#include <QHash>
#include <QString>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace ClearCase {
namespace Internal {

enum DiffType
{
    GraphicalDiff,
    ExternalDiff
};

class ClearCaseSettings
{
public:
    ClearCaseSettings();

    void fromSettings(QSettings *);
    void toSettings(QSettings *) const;

    inline int longTimeOutS() const { return timeOutS * 10; }

    bool equals(const ClearCaseSettings &s) const;

    QString ccCommand;
    QString ccBinaryPath;
    DiffType diffType = GraphicalDiff;
    QString diffArgs;
    QString indexOnlyVOBs;
    QHash<QString, int> totalFiles;
    bool autoAssignActivityName = true;
    bool autoCheckOut = true;
    bool noComment = false;
    bool keepFileUndoCheckout = true;
    bool promptToCheckIn = false;
    bool disableIndexer = false;
    bool extDiffAvailable = false;
    int historyCount;
    int timeOutS;
};

inline bool operator==(const ClearCaseSettings &p1, const ClearCaseSettings &p2)
{ return p1.equals(p2); }
inline bool operator!=(const ClearCaseSettings &p1, const ClearCaseSettings &p2)
{ return !p1.equals(p2); }

} // namespace Internal
} // namespace ClearCase
