/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CVSSETTINGS_H
#define CVSSETTINGS_H

#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace CVS {
namespace Internal {

struct CVSSettings
{
    CVSSettings();

    void fromSettings(QSettings *);
    void toSettings(QSettings *) const;

    inline int timeOutMS() const     { return timeOutS * 1000;  }
    inline int longTimeOutMS() const { return timeOutS * 10000; }

    // Add common options to the command line
    QStringList addOptions(const QStringList &args) const;

    bool equals(const CVSSettings &s) const;

    QString cvsCommand;
    QString cvsRoot;
    QString cvsDiffOptions;
    int timeOutS;
    bool promptToSubmit;
    bool describeByCommitId;
};

inline bool operator==(const CVSSettings &p1, const CVSSettings &p2)
    { return p1.equals(p2); }
inline bool operator!=(const CVSSettings &p1, const CVSSettings &p2)
    { return !p1.equals(p2); }

} // namespace Internal
} // namespace CVS

#endif // CVSSETTINGS_H
