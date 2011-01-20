/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "debugginghelper.h"

#include <coreplugin/icore.h>

#include <utils/synchronousprocess.h>

#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>
#include <QtCore/QHash>
#include <QtCore/QProcess>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QStringList>

#include <QtGui/QDesktopServices>

using namespace ProjectExplorer;

static inline QStringList validBinaryFilenames()
{
    return QStringList()
            << QLatin1String("debug/gdbmacros.dll")
            << QLatin1String("libgdbmacros.dylib")
            << QLatin1String("libgdbmacros.so");
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

QStringList DebuggingHelperLibrary::locationsByInstallData(const QString &qtInstallData)
{
    QStringList result;
    QFileInfo fileInfo;
    const QStringList binFilenames = validBinaryFilenames();
    foreach(const QString &directory, debuggingHelperLibraryDirectories(qtInstallData)) {
        if (getHelperFileInfoFor(binFilenames, directory, &fileInfo))
            result << fileInfo.filePath();
    }
    return result;
}

QString DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(const QString &qtInstallData)
{
    if (!Core::ICore::instance())
        return QString();

    const QString mainFilename = Core::ICore::instance()->resourcePath()
            + QLatin1String("/gdbmacros/gdbmacros.cpp");
    const QStringList directories = DebuggingHelperLibrary::debuggingHelperLibraryDirectories(qtInstallData);
    const QStringList binFilenames = validBinaryFilenames();

    return byInstallDataHelper(mainFilename, directories, binFilenames);
}

QString DebuggingHelperLibrary::copy(const QString &qtInstallData,
                                     QString *errorMessage)
{
    // Locations to try:
    //    $QTDIR/qtc-debugging-helper
    //    $APPLICATION-DIR/qtc-debugging-helper/$hash
    //    $USERDIR/qtc-debugging-helper/$hash
    const QStringList directories = DebuggingHelperLibrary::debuggingHelperLibraryDirectories(qtInstallData);

    QStringList files;
    files << QLatin1String("gdbmacros.cpp") << QLatin1String("gdbmacros_p.h")
          << QLatin1String("gdbmacros.h") << QLatin1String("gdbmacros.pro")
          << QLatin1String("LICENSE.LGPL") << QLatin1String("LGPL_EXCEPTION.TXT");

    QString sourcePath = Core::ICore::instance()->resourcePath() + QLatin1String("/gdbmacros/");

    // Try to find a writeable directory.
    foreach(const QString &directory, directories)
        if (copyFiles(sourcePath, files, directory, errorMessage)) {
            errorMessage->clear();
            return directory;
        }
    *errorMessage = QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary",
                                                "The debugger helpers could not be built in any of the directories:\n- %1\n\nReason: %2")
                    .arg(directories.join(QLatin1String("\n- ")), *errorMessage);
    return QString();
}

bool DebuggingHelperLibrary::build(const QString &directory, const QString &makeCommand,
                                      const QString &qmakeCommand, const QString &mkspec,
                                      const Utils::Environment &env, const QString &targetMode,
                                      QString *output, QString *errorMessage)
{
    return buildHelper(QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary",
                                                   "GDB helper"), QLatin1String("gdbmacros.pro"), directory,
                       makeCommand, qmakeCommand, mkspec, env, targetMode, output, errorMessage);
}
