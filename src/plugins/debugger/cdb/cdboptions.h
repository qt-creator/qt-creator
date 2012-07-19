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

#ifndef CDBSETTINGS_H
#define CDBSETTINGS_H

#include <QStringList>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

struct CdbOptions
{
public:
    CdbOptions();

    void clear();

    bool isValid() { return true; }

    void fromSettings(QSettings *s); // Writes parameters on first-time autodetect
    void toSettings(QSettings *s) const;

    bool equals(const CdbOptions &rhs) const;

    static QString settingsGroup();
    static QStringList oldEngineSymbolPaths(const QSettings *s);

    QString additionalArguments;
    QStringList symbolPaths;
    QStringList sourcePaths;
    // Events to break on (Command 'sxe' with abbreviation and optional parameter)
    QStringList breakEvents;
    // Launch CDB's own console instead of Qt Creator's
    bool cdbConsole;
    // Perform code-model based correction of breakpoint location.
    bool breakpointCorrection;
};

inline bool operator==(const CdbOptions &s1, const CdbOptions &s2)
{ return s1.equals(s2); }
inline bool operator!=(const CdbOptions &s1, const CdbOptions &s2)
{ return !s1.equals(s2); }

} // namespace Internal
} // namespace Debugger

#endif // CDBSETTINGS_H
