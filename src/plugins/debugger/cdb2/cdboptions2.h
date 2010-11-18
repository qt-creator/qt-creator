/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CDBSETTINGS_H
#define CDBSETTINGS_H

#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Debugger {
namespace Cdb {

struct CdbOptions
{
public:
    CdbOptions();
    void clear();

    void fromSettings(const QSettings *s);
    void toSettings(QSettings *s) const;

    bool equals(const CdbOptions &rhs) const;

    static bool autoDetectExecutable(QString *outPath, bool *is64bit = 0,
                                     QStringList *checkedDirectories = 0);

    static QString settingsGroup();

    bool enabled;
    bool is64bit;
    QString executable;
    QStringList symbolPaths;
    QStringList sourcePaths;
};

inline bool operator==(const CdbOptions &s1, const CdbOptions &s2)
{ return s1.equals(s2); }
inline bool operator!=(const CdbOptions &s1, const CdbOptions &s2)
{ return !s1.equals(s2); }

} // namespace Internal
} // namespace Debugger

#endif // CDBSETTINGS_H
