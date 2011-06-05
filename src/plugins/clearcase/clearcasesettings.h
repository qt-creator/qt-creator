/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 AudioCodes Ltd.
**
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef CLEARCASESETTINGS_H
#define CLEARCASESETTINGS_H

#include <QHash>
#include <QStringList>

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

    inline int timeOutMS() const     { return timeOutS * 1000;  }
    inline int longTimeOutMS() const { return timeOutS * 10000; }

    bool equals(const ClearCaseSettings &s) const;

    QString ccCommand;
    int historyCount;
    int timeOutS;
    DiffType diffType;
    QString diffArgs;
    bool autoAssignActivityName;
    bool autoCheckOut;
    bool promptToCheckIn;
    bool disableIndexer;
    QString indexOnlyVOBs;
    QHash<QString, int> totalFiles;
};

inline bool operator==(const ClearCaseSettings &p1, const ClearCaseSettings &p2)
    { return p1.equals(p2); }
inline bool operator!=(const ClearCaseSettings &p1, const ClearCaseSettings &p2)
    { return !p1.equals(p2); }

} // namespace Internal
} // namespace ClearCase

#endif // CLEARCASESETTINGS_H
