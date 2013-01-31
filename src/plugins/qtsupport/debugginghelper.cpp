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

#include "debugginghelper.h"

#include <coreplugin/icore.h>

#include <utils/synchronousprocess.h>

#include <QFileInfo>
#include <QCoreApplication>
#include <QHash>
#include <QProcess>
#include <QDir>
#include <QDateTime>
#include <QStringList>

#include <QDesktopServices>

using namespace QtSupport;

static inline QStringList validBinaryFilenames()
{
    return QStringList()
            << QLatin1String("debug/dumper.dll")
            << QLatin1String("libdumper.dylib")
            << QLatin1String("libdumper.so");
}

QStringList DebuggingHelperLibrary::debuggingHelperLibraryDirectories(const QString &qtInstallData)
{
    const QChar slash = QLatin1Char('/');
    const uint hash = qHash(qtInstallData);
    QStringList directories;
    directories
            << (qtInstallData + QLatin1String("/qtc-debugging-helper/"))
            << QDir::cleanPath((QCoreApplication::applicationDirPath() + QLatin1String("/../qtc-debugging-helper/") + QString::number(hash))) + slash
            << (QDesktopServices::storageLocation(QDesktopServices::DataLocation) + QLatin1String("/qtc-debugging-helper/") + QString::number(hash)) + slash;
    return directories;
}

static QString sourcePath()
{
    return Core::ICore::resourcePath() + QLatin1String("/dumper/");
}

static QStringList sourceFileNames()
{
    return QStringList()
            << QLatin1String("dumper.cpp") << QLatin1String("dumper_p.h")
            << QLatin1String("dumper.h") << QLatin1String("dumper.pro")
            << QLatin1String("LICENSE.LGPL") << QLatin1String("LGPL_EXCEPTION.TXT");
}

QString DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(const QString &qtInstallData)
{
    if (!Core::ICore::instance())
        return QString();

    const QStringList directories = DebuggingHelperLibrary::debuggingHelperLibraryDirectories(qtInstallData);
    const QStringList binFilenames = validBinaryFilenames();

    return byInstallDataHelper(sourcePath(), sourceFileNames(), directories, binFilenames, false);
}

QString DebuggingHelperLibrary::copy(const QString &qtInstallData,
                                     QString *errorMessage)
{
    // Locations to try:
    //    $QTDIR/qtc-debugging-helper
    //    $APPLICATION-DIR/qtc-debugging-helper/$hash
    //    $USERDIR/qtc-debugging-helper/$hash
    const QStringList directories = DebuggingHelperLibrary::debuggingHelperLibraryDirectories(qtInstallData);

    // Try to find a writeable directory.
    foreach (const QString &directory, directories)
        if (copyFiles(sourcePath(), sourceFileNames(), directory, errorMessage)) {
            errorMessage->clear();
            return directory;
        }
    *errorMessage = QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary",
                                                "The debugger helpers could not be built in any of the directories:\n- %1\n\nReason: %2")
                    .arg(directories.join(QLatin1String("\n- ")), *errorMessage);
    return QString();
}

bool DebuggingHelperLibrary::build(BuildHelperArguments arguments, QString *log, QString *errorMessage)
{
    arguments.proFilename = QLatin1String("dumper.pro");
    arguments.helperName = QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary",
                                                       "GDB helper");
    return buildHelper(arguments, log, errorMessage);
}
