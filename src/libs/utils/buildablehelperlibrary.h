/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "environment.h"
#include "fileutils.h"

QT_FORWARD_DECLARE_CLASS(QFileInfo)

namespace Utils {

class QTCREATOR_UTILS_EXPORT BuildableHelperLibrary
{
public:
    // returns the full path to the first qmake, qmake-qt4, qmake4 that has
    // at least version 2.0.0 and thus is a qt4 qmake
    static FileName findSystemQt(const Environment &env);
    static bool isQtChooser(const QFileInfo &info);
    static QString qtChooserToQmakePath(const QString &path);
    // return true if the qmake at qmakePath is a Qt (used by QtVersion)
    static QString qtVersionForQMake(const QString &qmakePath);
    // returns something like qmake4, qmake, qmake-qt4 or whatever distributions have chosen (used by QtVersion)
    static QStringList possibleQMakeCommands();
    static QString filterForQmakeFileDialog();

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
        Environment environment;

        FileName qmakeCommand;
        QString targetMode;
        FileName mkspec;
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
