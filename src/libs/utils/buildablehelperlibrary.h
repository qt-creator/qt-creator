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

#include "utils_global.h"

#include "filepath.h"

#include <QCoreApplication>

namespace Utils {

class Environment;

class QTCREATOR_UTILS_EXPORT BuildableHelperLibrary
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::DebuggingHelperLibrary)

public:
    // returns the full path to the first qmake, qmake-qt4, qmake4 that has
    // at least version 2.0.0 and thus is a qt4 qmake
    static FilePath findSystemQt(const Environment &env);
    static FilePaths findQtsInEnvironment(const Environment &env, int maxCount = -1);
    static bool isQtChooser(const FilePath &filePath);
    static FilePath qtChooserToQmakePath(const FilePath &path);
    // return true if the qmake at qmakePath is a Qt (used by QtVersion)
    static QString qtVersionForQMake(const FilePath &qmakePath);
    // returns something like qmake4, qmake, qmake-qt4 or whatever distributions have chosen (used by QtVersion)
    static QStringList possibleQMakeCommands();
    static QString filterForQmakeFileDialog();
};

} // Utils
