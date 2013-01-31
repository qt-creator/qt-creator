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

#ifndef CORE_FILEUTILS_H
#define CORE_FILEUTILS_H

#include "coreplugin/core_global.h"

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Core {

struct CORE_EXPORT FileUtils
{
    // Helpers for common directory browser options.
    static void showInGraphicalShell(QWidget *parent, const QString &path);
    static void openTerminal(const QString &path);
    // Platform-dependent action descriptions
    static QString msgGraphicalShellAction();
    static QString msgTerminalAction();
    // File operations aware of version control and file system case-insensitiveness
    static void removeFile(const QString &filePath, bool deleteFromFS);
    static bool renameFile(const QString &from, const QString &to);
};

} // namespace Core

#endif // CORE_FILEUTILS_H
