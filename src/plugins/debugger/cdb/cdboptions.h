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

#ifndef CDBSETTINGS_H
#define CDBSETTINGS_H

#include <QtCore/QStringList>

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

    void fromSettings(const QSettings *s);
    void toSettings(QSettings *s) const;

    // A set of flags for comparison function.
    enum ChangeFlags { InitializationOptionsChanged = 0x1,
                       DebuggerPathsChanged = 0x2,
                       SymbolOptionsChanged = 0x4,
                       FastLoadDebuggingHelpersChanged = 0x8,
                       OtherOptionsChanged = 0x100
                     };
    unsigned compare(const CdbOptions &s) const;

    // Format a symbol server specification with a cache directory
    static QString symbolServerPath(const QString &cacheDir);
    // Check whether the path is a symbol server specification and return the cache directory
    static int indexOfSymbolServerPath(const QStringList &symbolPaths, QString *cacheDir = 0);

    static QString settingsGroup();

    bool enabled;
    QString path;
    QStringList symbolPaths;
    QStringList sourcePaths;
    bool breakOnException;
    bool verboseSymbolLoading;
    bool fastLoadDebuggingHelpers;
};

inline bool operator==(const CdbOptions &s1, const CdbOptions &s2)
{ return s1.compare(s2) == 0u; }
inline bool operator!=(const CdbOptions &s1, const CdbOptions &s2)
{ return s1.compare(s2) != 0u; }

} // namespace Internal
} // namespace Debugger

#endif // CDBSETTINGS_H
