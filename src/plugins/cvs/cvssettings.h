/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CVSSETTINGS_H
#define CVSSETTINGS_H

#include <QStringList>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Cvs {
namespace Internal {

struct CvsSettings
{
    CvsSettings();

    void fromSettings(QSettings *);
    void toSettings(QSettings *) const;

    int timeOutMS() const { return timeOutS * 1000;  }
    int longTimeOutMS() const { return timeOutS * 10000; }

    // Add common options to the command line
    QStringList addOptions(const QStringList &args) const;

    bool equals(const CvsSettings &s) const;

    QString cvsCommand;
    QString cvsRoot;
    QString cvsDiffOptions;
    int timeOutS;
    bool promptToSubmit;
    bool describeByCommitId;
};

inline bool operator==(const CvsSettings &p1, const CvsSettings &p2)
    { return p1.equals(p2); }
inline bool operator!=(const CvsSettings &p1, const CvsSettings &p2)
    { return !p1.equals(p2); }

} // namespace Internal
} // namespace Cvs

#endif // CVSSETTINGS_H
