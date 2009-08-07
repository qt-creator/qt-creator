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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "debugginghelper.h"
#include <coreplugin/icore.h>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QProcess>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtGui/QApplication>
#include <QtGui/QDesktopServices>

using namespace ProjectExplorer;

QString DebuggingHelperLibrary::findSystemQt(const Environment &env)
{
    QStringList paths = env.path();
    foreach (const QString &path, paths) {
        foreach (const QString &possibleCommand, possibleQMakeCommands()) {
            QFileInfo qmake(path + "/" + possibleCommand);
            if (qmake.exists()) {
                if (!qtVersionForQMake(qmake.absoluteFilePath()).isNull()) {
                    return qmake.absoluteFilePath();
                }
            }
        }
    }
    return QString::null;
}

bool DebuggingHelperLibrary::hasDebuggingHelperLibrary(const QString &qmakePath)
{
    return !debuggingHelperLibrary(qmakePath).isNull();
}

QStringList DebuggingHelperLibrary::debuggingHelperLibraryDirectories(const QString &qtInstallData, const QString &qtpath)
{
    uint hash = qHash(qtpath);
    QStringList directories;
    directories
            << (qtInstallData + "/qtc-debugging-helper/")
            << QDir::cleanPath((QApplication::applicationDirPath() + "/../qtc-debugging-helper/" + QString::number(hash))) + "/"
            << (QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/qtc-debugging-helper/" + QString::number(hash)) + "/";
    return directories;
}

QStringList DebuggingHelperLibrary::debuggingHelperLibraryLocations(const QString &qmakePath)
{
    return debuggingHelperLibraryLocations(qtInstallDataDir(qmakePath), qtDir(qmakePath));
}

QString DebuggingHelperLibrary::debuggingHelperLibrary(const QString &qmakePath)
{
    return debuggingHelperLibrary(qtInstallDataDir(qmakePath), qtDir(qmakePath));
}

QString DebuggingHelperLibrary::qtInstallDataDir(const QString &qmakePath)
{
    QProcess proc;
    proc.start(qmakePath, QStringList() << "-query"<< "QT_INSTALL_DATA");
    if (proc.waitForFinished())
        return QString(proc.readAll().trimmed());
    return QString::null;
}

QString DebuggingHelperLibrary::qtDir(const QString &qmakePath)
{
    QDir dir = QFileInfo(qmakePath).absoluteDir();
    dir.cdUp();
    return dir.absolutePath();
}

// Debugging Helper Library

QStringList DebuggingHelperLibrary::debuggingHelperLibraryLocations(const QString &qtInstallData, const QString &qtpath)
{
    QStringList result;
    foreach(const QString &directory, debuggingHelperLibraryDirectories(qtInstallData, qtpath)) {
#if defined(Q_OS_WIN)
        QFileInfo fi(directory + "debug/gdbmacros.dll");
#elif defined(Q_OS_MAC)
        QFileInfo fi(directory + "libgdbmacros.dylib");
#else // generic UNIX
        QFileInfo fi(directory + "libgdbmacros.so");
#endif
        result << fi.filePath();
    }
    return result;
}

QString DebuggingHelperLibrary::debuggingHelperLibrary(const QString &qtInstallData, const QString &qtpath)
{
    foreach(const QString &directory, debuggingHelperLibraryDirectories(qtInstallData, qtpath)) {
#if defined(Q_OS_WIN)
        QFileInfo fi(directory + "debug/gdbmacros.dll");
#elif defined(Q_OS_MAC)
        QFileInfo fi(directory + "libgdbmacros.dylib");
#else // generic UNIX
        QFileInfo fi(directory + "libgdbmacros.so");
#endif
        if (fi.exists())
            return fi.filePath();
    }
    return QString();
}


QString DebuggingHelperLibrary::buildDebuggingHelperLibrary(const QString &qmakePath, const QString &make, const Environment &env)
{
    QString directory = copyDebuggingHelperLibrary(qtInstallDataDir(qmakePath), qtDir(qmakePath));
    if (directory.isEmpty())
        return QString::null;
    return buildDebuggingHelperLibrary(directory, make, qmakePath, QString::null, env);
}

QString DebuggingHelperLibrary::copyDebuggingHelperLibrary(const QString &qtInstallData, const QString &qtdir)
{
    // Locations to try:
    //    $QTDIR/qtc-debugging-helper
    //    $APPLICATION-DIR/qtc-debugging-helper/$hash
    //    $USERDIR/qtc-debugging-helper/$hash
    QStringList directories = DebuggingHelperLibrary::debuggingHelperLibraryDirectories(qtInstallData, qtdir);

    QStringList files;
    files << "gdbmacros.cpp" << "gdbmacros.pro"
          << "LICENSE.LGPL" << "LGPL_EXCEPTION.TXT";
    foreach(const QString &directory, directories) {
        QString dumperPath = Core::ICore::instance()->resourcePath() + "/gdbmacros/";
        bool success = true;
        QDir().mkpath(directory);
        foreach (const QString &file, files) {
            QString source = dumperPath + file;
            QString dest = directory + file;
            QFileInfo destInfo(dest);
            if (destInfo.exists()) {
                if (destInfo.lastModified() >= QFileInfo(source).lastModified())
                    continue;
                success &= QFile::remove(dest);
            }
            success &= QFile::copy(source, dest);
        }
        if (success)
            return directory;
    }
    return QString::null;
}

QString DebuggingHelperLibrary::buildDebuggingHelperLibrary(const QString &directory, const QString &makeCommand, const QString &qmakeCommand, const QString &mkspec, const Environment &env)
{
    QString output;
    // Setup process
    QProcess proc;
    proc.setEnvironment(env.toStringList());
    proc.setWorkingDirectory(directory);
    proc.setProcessChannelMode(QProcess::MergedChannels);

    output += QString("Building debugging helper library in %1\n").arg(directory);
    output += "\n";

    QString makeFullPath = env.searchInPath(makeCommand);
    if (QFileInfo(directory + "/Makefile").exists()) {
        if (!makeFullPath.isEmpty()) {
            output += QString("Running %1 clean...\n").arg(makeFullPath);
            proc.start(makeFullPath, QStringList() << "clean");
            proc.waitForFinished();
            output += proc.readAll();
        } else {
            output += QString("%1 not found in PATH\n").arg(makeCommand);
            return output;
        }
    }

    output += QString("\nRunning %1 ...\n").arg(qmakeCommand);

    proc.start(qmakeCommand, QStringList()<<"-spec"<< (mkspec.isEmpty() ? "default" : mkspec) <<"gdbmacros.pro");
    proc.waitForFinished();

    output += proc.readAll();

    output += "\n";
    if (!makeFullPath.isEmpty()) {
        output += QString("Running %1 ...\n").arg(makeFullPath);
        proc.start(makeFullPath, QStringList());
        proc.waitForFinished();
        output += proc.readAll();
    } else {
        output += QString("%1 not found in PATH\n").arg(makeCommand);
    }
    return output;
}

QString DebuggingHelperLibrary::qtVersionForQMake(const QString &qmakePath)
{
    QProcess qmake;
    qmake.start(qmakePath, QStringList()<<"--version");
    if (!qmake.waitForFinished())
        return false;
    QString output = qmake.readAllStandardOutput();
    QRegExp regexp("(QMake version|QMake version:)[\\s]*([\\d.]*)", Qt::CaseInsensitive);
    regexp.indexIn(output);
    if (regexp.cap(2).startsWith("2.")) {
        QRegExp regexp2("Using Qt version[\\s]*([\\d\\.]*)", Qt::CaseInsensitive);
        regexp2.indexIn(output);
        return regexp2.cap(1);
    }
    return QString();
}

QStringList DebuggingHelperLibrary::possibleQMakeCommands()
{
    // On windows noone has renamed qmake, right?
#ifdef Q_OS_WIN
    return QStringList() << "qmake.exe";
#else
    // On unix some distributions renamed qmake to avoid clashes
    QStringList result;
    result << "qmake-qt4" << "qmake4" << "qmake";
    return result;
#endif
}

