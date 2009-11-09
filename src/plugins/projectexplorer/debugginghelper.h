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

#ifndef DEBUGGINGHELPER_H
#define DEBUGGINGHELPER_H

#include "environment.h"
#include "projectexplorer_export.h"

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace ProjectExplorer {
class PROJECTEXPLORER_EXPORT DebuggingHelperLibrary
{
public:
    // returns the full path to the first qmake, qmake-qt4, qmake4 that has
    // at least version 2.0.0 and thus is a qt4 qmake
    static QString findSystemQt(const Environment &env);
    // returns something like qmake4, qmake, qmake-qt4 or whatever distributions have chosen (used by QtVersion)
    static QStringList possibleQMakeCommands();
    // return true if the qmake at qmakePath is qt4 (used by QtVersion)
    static QString qtVersionForQMake(const QString &qmakePath);

    static QString debuggingHelperLibraryByInstallData(const QString &qtInstallData);
    static QStringList debuggingHelperLibraryLocationsByInstallData(const QString &qtInstallData);

    // Build the helpers and return the output log/errormessage.
    static QString buildDebuggingHelperLibrary(const QString &qmakePath, const QString &make, const Environment &env);
    static QString buildDebuggingHelperLibrary(const QString &directory, const QString &makeCommand, const QString &qmakeCommand, const QString &mkspec, const Environment &env);

    // Copy the source files to a target location and return the chosen target location.
    static QString copyDebuggingHelperLibrary(const QString &qtInstallData, QString *errorMessage);
    static QString qtInstallDataDir(const QString &qmakePath);
private:
    static QStringList debuggingHelperLibraryDirectories(const QString &qtInstallData);
};
}

#endif // DEBUGGINGHELPER_H
