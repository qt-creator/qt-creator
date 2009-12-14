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

    inline int timeOutMS() const     { return timeOutS * 10000;  }
    inline int longTimeOutMS() const { return timeOutS * 100000; }

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
