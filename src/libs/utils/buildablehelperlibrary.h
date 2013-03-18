/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BUILDABLEHELPERLIBRARY_H
#define BUILDABLEHELPERLIBRARY_H

#include "environment.h"
#include "fileutils.h"

QT_FORWARD_DECLARE_CLASS(QFileInfo)

namespace Utils {

class QTCREATOR_UTILS_EXPORT BuildableHelperLibrary
{
public:
    // returns the full path to the first qmake, qmake-qt4, qmake4 that has
    // at least version 2.0.0 and thus is a qt4 qmake
    static FileName findSystemQt(const Utils::Environment &env);
    // return true if the qmake at qmakePath is qt4 (used by QtVersion)
    static QString qtVersionForQMake(const QString &qmakePath);
    static QString qtVersionForQMake(const QString &qmakePath, bool *qmakeIsExecutable);
    // returns something like qmake4, qmake, qmake-qt4 or whatever distributions have chosen (used by QtVersion)
    static QStringList possibleQMakeCommands();

    static QString qtInstallHeadersDir(const QString &qmakePath);
    static QString qtInstallDataDir(const FileName &qmakePath);

    static QString byInstallDataHelper(const QString &sourcePath,
                                       const QStringList &sourceFileNames,
                                       const QStringList &installDirectories,
                                       const QStringList &validBinaryFilenames,
                                       bool acceptOutdatedHelper);

    static bool copyFiles(const QString &sourcePath, const QStringList &files,
                          const QString &targetDirectory, QString *errorMessage);

    struct BuildHelperArguments {
        QString helperName;
        QString directory;
        Utils::Environment environment;

        Utils::FileName qmakeCommand;
        QString targetMode;
        Utils::FileName mkspec;
        QString proFilename;
        QStringList qmakeArguments;

        QString makeCommand;
        QStringList makeArguments;
    };

    static bool buildHelper(const BuildHelperArguments &arguments,
                            QString *log, QString *errorMessage);

    static bool getHelperFileInfoFor(const QStringList &validBinaryFilenames,
                                     const QString &directory, QFileInfo* info);
};

}

#endif // BUILDABLEHELPERLIBRARY_H
