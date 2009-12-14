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

#include "debugginghelper.h"

#include <coreplugin/icore.h>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>
#include <QtCore/QHash>
#include <QtCore/QProcess>
#include <QtCore/QDir>
#include <QtCore/QDateTime>

#include <QtGui/QDesktopServices>

using namespace ProjectExplorer;

QString DebuggingHelperLibrary::findSystemQt(const Environment &env)
{
    QStringList paths = env.path();
    foreach (const QString &path, paths) {
        foreach (const QString &possibleCommand, possibleQMakeCommands()) {
            const QFileInfo qmake(path + QLatin1Char('/') + possibleCommand);
            if (qmake.exists()) {
                if (!qtVersionForQMake(qmake.absoluteFilePath()).isNull()) {
                    return qmake.absoluteFilePath();
                }
            }
        }
    }
    return QString::null;
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

QString DebuggingHelperLibrary::qtInstallDataDir(const QString &qmakePath)
{
    QProcess proc;
    proc.start(qmakePath, QStringList() << QLatin1String("-query") << QLatin1String("QT_INSTALL_DATA"));
    if (proc.waitForFinished())
        return QString(proc.readAll().trimmed());
    return QString::null;
}

// Debugging Helper Library

static inline bool getHelperFileInfoFor(const QString &directory, QFileInfo* info)
{
    if (!info)
        return false;

    info->setFile(directory + QLatin1String("debug/gdbmacros.dll"));
    if (info->exists())
        return true;

    info->setFile(directory + QLatin1String("libgdbmacros.dylib"));
    if (info->exists())
        return true;

    info->setFile(directory + QLatin1String("libgdbmacros.so"));
    if (info->exists())
        return true;

    return false;
}

QStringList DebuggingHelperLibrary::debuggingHelperLibraryLocationsByInstallData(const QString &qtInstallData)
{
    QStringList result;
    QFileInfo fileInfo;
    foreach(const QString &directory, debuggingHelperLibraryDirectories(qtInstallData)) {
        if (getHelperFileInfoFor(directory, &fileInfo))
            result << fileInfo.filePath();
    }
    return result;
}

QString DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(const QString &qtInstallData)
{
    const QString dumperSourcePath = Core::ICore::instance()->resourcePath() + QLatin1String("/gdbmacros/");
    QDateTime lastModified = QFileInfo(dumperSourcePath + "gdbmacros.cpp").lastModified();
    // We pretend that the lastmodified of gdbmacros.cpp is 5 minutes before what the file system says
    // Because afer a installation from the package the modified dates of gdbmacros.cpp
    // and the actual library are close to each other, but not deterministic in one direction
    lastModified = lastModified.addSecs(-300);

    QFileInfo fileInfo;
    foreach(const QString &directory, debuggingHelperLibraryDirectories(qtInstallData)) {
        if (getHelperFileInfoFor(directory, &fileInfo)) {
            if (fileInfo.lastModified() >= lastModified)
                return fileInfo.filePath();
        }
    }
    return QString();
}

QString DebuggingHelperLibrary::buildDebuggingHelperLibrary(const QString &qmakePath, const QString &make, const Environment &env)
{
    QString errorMessage;
    const QString directory = copyDebuggingHelperLibrary(qtInstallDataDir(qmakePath), &errorMessage);
    if (directory.isEmpty())
        return errorMessage;
    return buildDebuggingHelperLibrary(directory, make, qmakePath, QString::null, env);
}

// Copy helper source files to a target directory, replacing older files.
static bool copyDebuggingHelperFiles(const QStringList &files,
                                     const QString &targetDirectory,
                                     QString *errorMessage)
{
    const QString dumperSourcePath = Core::ICore::instance()->resourcePath() + QLatin1String("/gdbmacros/");
    if (!QDir().mkpath(targetDirectory)) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary", "The target directory %1 could not be created.").arg(targetDirectory);
        return false;
    }
    foreach (const QString &file, files) {
        const QString source = dumperSourcePath + file;
        const QString dest = targetDirectory + file;
        const QFileInfo destInfo(dest);
        if (destInfo.exists()) {
            if (destInfo.lastModified() >= QFileInfo(source).lastModified())
                continue;
            if (!QFile::remove(dest)) {
                *errorMessage = QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary", "The existing file %1 could not be removed.").arg(destInfo.absoluteFilePath());
                return false;
            }
        }
        if (!QFile::copy(source, dest)) {
            *errorMessage = QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary", "The file %1 could not be copied to %2.").arg(source, dest);
            return false;
        }
    }
    return true;
}

QString DebuggingHelperLibrary::copyDebuggingHelperLibrary(const QString &qtInstallData,
                                                           QString *errorMessage)
{
    // Locations to try:
    //    $QTDIR/qtc-debugging-helper
    //    $APPLICATION-DIR/qtc-debugging-helper/$hash
    //    $USERDIR/qtc-debugging-helper/$hash
    const QStringList directories = DebuggingHelperLibrary::debuggingHelperLibraryDirectories(qtInstallData);

    QStringList files;
    files << QLatin1String("gdbmacros.cpp") << QLatin1String("gdbmacros_p.h") << QLatin1String("gdbmacros.h") << QLatin1String("gdbmacros.pro")
          << QLatin1String("LICENSE.LGPL") << QLatin1String("LGPL_EXCEPTION.TXT");
    // Try to find a writeable directory.
    foreach(const QString &directory, directories)
        if (copyDebuggingHelperFiles(files, directory, errorMessage)) {
            errorMessage->clear();
            return directory;
        }
    *errorMessage = QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary", "The debugger helpers could not be built in any of the directories:\n- %1\n\nReason: %2")
                    .arg(directories.join(QLatin1String("\n- ")), *errorMessage);
    return QString();
}

QString DebuggingHelperLibrary::buildDebuggingHelperLibrary(const QString &directory, const QString &makeCommand, const QString &qmakeCommand, const QString &mkspec, const Environment &env)
{
    QString output;
    const QChar newline = QLatin1Char('\n');
    // Setup process
    QProcess proc;
    proc.setEnvironment(env.toStringList());
    proc.setWorkingDirectory(directory);
    proc.setProcessChannelMode(QProcess::MergedChannels);

    output += QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary", "Building debugging helper library in %1\n").arg(directory);
    output += newline;

    const QString makeFullPath = env.searchInPath(makeCommand);
    if (QFileInfo(directory + QLatin1String("/Makefile")).exists()) {
        if (!makeFullPath.isEmpty()) {
            const QString cleanTarget = QLatin1String("distclean");
            output += QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary", "Running %1 %2...\n").arg(makeFullPath, cleanTarget);
            proc.start(makeFullPath, QStringList(cleanTarget));
            proc.waitForFinished();
            output += QString::fromLocal8Bit(proc.readAll());
        } else {
            output += QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary", "%1 not found in PATH\n").arg(makeCommand);
            return output;
        }
    }
    output += newline;
    output += QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary", "Running %1 ...\n").arg(qmakeCommand);

    QStringList makeArgs;
    makeArgs << QLatin1String("-spec")<< (mkspec.isEmpty() ? QString(QLatin1String("default")) : mkspec) << QLatin1String("gdbmacros.pro");
    proc.start(qmakeCommand, makeArgs);
    proc.waitForFinished();

    output += proc.readAll();

    output += newline;;
    if (!makeFullPath.isEmpty()) {
        output += QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary", "Running %1 ...\n").arg(makeFullPath);
        proc.start(makeFullPath, QStringList());
        proc.waitForFinished();
        output += proc.readAll();
    } else {
        output += QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary", "%1 not found in PATH\n").arg(makeCommand);
    }
    return output;
}

QString DebuggingHelperLibrary::qtVersionForQMake(const QString &qmakePath)
{
    QProcess qmake;
    qmake.start(qmakePath, QStringList(QLatin1String("--version")));
    if (!qmake.waitForFinished())
        return QString::null;
    QString output = qmake.readAllStandardOutput();
    QRegExp regexp(QLatin1String("(QMake version|QMake version:)[\\s]*([\\d.]*)"), Qt::CaseInsensitive);
    regexp.indexIn(output);
    if (regexp.cap(2).startsWith(QLatin1String("2."))) {
        QRegExp regexp2(QLatin1String("Using Qt version[\\s]*([\\d\\.]*)"), Qt::CaseInsensitive);
        regexp2.indexIn(output);
        return regexp2.cap(1);
    }
    return QString();
}

QStringList DebuggingHelperLibrary::possibleQMakeCommands()
{
    // On windows noone has renamed qmake, right?
#ifdef Q_OS_WIN
    return QStringList(QLatin1String("qmake.exe"));
#else
    // On unix some distributions renamed qmake to avoid clashes
    QStringList result;
    result << QLatin1String("qmake-qt4") << QLatin1String("qmake4") << QLatin1String("qmake");
    return result;
#endif
}
