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

#include "buildablehelperlibrary.h"

#include <QFileInfo>
#include <QCoreApplication>
#include <QHash>
#include <QProcess>
#include <QDir>
#include <QDateTime>

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/synchronousprocess.h>

#include <QDesktopServices>
#include <QDebug>

namespace Utils {

Utils::FileName BuildableHelperLibrary::findSystemQt(const Utils::Environment &env)
{
    QStringList paths = env.path();
    foreach (const QString &path, paths) {
        QString prefix = path;
        if (!prefix.endsWith(QLatin1Char('/')))
            prefix.append(QLatin1Char('/'));
        foreach (const QString &possibleCommand, possibleQMakeCommands()) {
            const QFileInfo qmake(prefix + possibleCommand);
            if (qmake.exists()) {
                if (!qtVersionForQMake(qmake.absoluteFilePath()).isNull())
                    return Utils::FileName(qmake);
            }
        }
    }
    return Utils::FileName();
}

QString BuildableHelperLibrary::qtInstallDataDir(const Utils::FileName &qmakePath)
{
    QProcess proc;
    proc.start(qmakePath.toString(), QStringList() << QLatin1String("-query") << QLatin1String("QT_INSTALL_DATA"));
    if (proc.waitForFinished())
        return QString::fromLocal8Bit(proc.readAll()).trimmed();
    return QString();
}

QString BuildableHelperLibrary::qtVersionForQMake(const QString &qmakePath)
{
    bool qmakeIsExecutable;
    return BuildableHelperLibrary::qtVersionForQMake(qmakePath, &qmakeIsExecutable);
}

QString BuildableHelperLibrary::qtVersionForQMake(const QString &qmakePath, bool *qmakeIsExecutable)
{
    *qmakeIsExecutable = !qmakePath.isEmpty();
    if (!*qmakeIsExecutable)
        return QString();

    QProcess qmake;
    qmake.start(qmakePath, QStringList(QLatin1String("--version")));
    if (!qmake.waitForStarted()) {
        *qmakeIsExecutable = false;
        qWarning("Cannot start '%s': %s", qPrintable(qmakePath), qPrintable(qmake.errorString()));
        return QString();
    }
    if (!qmake.waitForFinished())      {
        Utils::SynchronousProcess::stopProcess(qmake);
        qWarning("Timeout running '%s'.", qPrintable(qmakePath));
        return QString();
    }
    if (qmake.exitStatus() != QProcess::NormalExit) {
        *qmakeIsExecutable = false;
        qWarning("'%s' crashed.", qPrintable(qmakePath));
        return QString();
    }

    const QString output = QString::fromLocal8Bit(qmake.readAllStandardOutput());
    static QRegExp regexp(QLatin1String("(QMake version|QMake version:)[\\s]*([\\d.]*)"),
                          Qt::CaseInsensitive);
    regexp.indexIn(output);
    const QString qmakeVersion = regexp.cap(2);
    if (qmakeVersion.startsWith(QLatin1String("2."))
            || qmakeVersion.startsWith(QLatin1String("3."))) {
        static QRegExp regexp2(QLatin1String("Using Qt version[\\s]*([\\d\\.]*)"),
                               Qt::CaseInsensitive);
        regexp2.indexIn(output);
        const QString version = regexp2.cap(1);
        return version;
    }
    return QString();
}

QStringList BuildableHelperLibrary::possibleQMakeCommands()
{
    // On windows no one has renamed qmake, right?
    if (HostOsInfo::isWindowsHost())
        return QStringList(QLatin1String("qmake.exe"));

    // On unix some distributions renamed qmake to avoid clashes
    QStringList result;
    result << QLatin1String("qmake-qt4") << QLatin1String("qmake4")
           << QLatin1String("qmake-qt5") << QLatin1String("qmake5") << QLatin1String("qmake");
    return result;
}

// Copy helper source files to a target directory, replacing older files.
bool BuildableHelperLibrary::copyFiles(const QString &sourcePath,
                                     const QStringList &files,
                                     const QString &targetDirectory,
                                     QString *errorMessage)
{
    // try remove the directory
    if (!FileUtils::removeRecursively(FileName::fromString(targetDirectory), errorMessage))
        return false;
    if (!QDir().mkpath(targetDirectory)) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary", "The target directory %1 could not be created.").arg(targetDirectory);
        return false;
    }
    foreach (const QString &file, files) {
        const QString source = sourcePath + file;
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
        if (!destInfo.dir().exists())
            QDir().mkpath(destInfo.dir().absolutePath());

        if (!QFile::copy(source, dest)) {
            *errorMessage = QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary", "The file %1 could not be copied to %2.").arg(source, dest);
            return false;
        }
    }
    return true;
}

// Helper: Run a build process with merged stdout/stderr
static inline bool runBuildProcessI(QProcess &proc,
                                    const QString &binary,
                                    const QStringList &args,
                                    int timeoutMS,
                                    bool ignoreNonNullExitCode,
                                    QString *output, QString *errorMessage)
{
    proc.start(binary, args);
    if (!proc.waitForStarted()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                                    "Cannot start process: %1").
                                                    arg(proc.errorString());
        return false;
    }
    // Read stdout/err and check for timeouts
    QByteArray stdOut;
    QByteArray stdErr;
    if (!SynchronousProcess::readDataFromProcess(proc, timeoutMS, &stdOut, &stdErr, false)) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                                    "Timeout after %1s.").
                                                    arg(timeoutMS / 1000);
        SynchronousProcess::stopProcess(proc);
        return false;
    }
    if (proc.exitStatus() != QProcess::NormalExit) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                                    "The process crashed.");
        return false;
    }
    const QString stdOutS = QString::fromLocal8Bit(stdOut);
    if (!ignoreNonNullExitCode && proc.exitCode() != 0) {
            *errorMessage = QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                                        "The process returned exit code %1:\n%2").
                                                         arg(proc.exitCode()).arg(stdOutS);
        return false;
    }
    output->append(stdOutS);
    return true;
}

// Run a build process with merged stdout/stderr and qWarn about errors.
static bool runBuildProcess(QProcess &proc,
                            const QString &binary,
                            const QStringList &args,
                            int timeoutMS,
                            bool ignoreNonNullExitCode,
                            QString *output, QString *errorMessage)
{
    const bool rc = runBuildProcessI(proc, binary, args, timeoutMS, ignoreNonNullExitCode, output, errorMessage);
    if (!rc) {
        // Fail - reformat error.
        QString cmd = binary;
        if (!args.isEmpty()) {
            cmd += QLatin1Char(' ');
            cmd += args.join(QString(QLatin1Char(' ')));
        }
        *errorMessage =
                QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                            "Error running '%1' in %2: %3").
                                            arg(cmd, proc.workingDirectory(), *errorMessage);
        qWarning("%s", qPrintable(*errorMessage));
    }
    return rc;
}


bool BuildableHelperLibrary::buildHelper(const BuildHelperArguments &arguments,
                                         QString *log, QString *errorMessage)
{
    const QChar newline = QLatin1Char('\n');
    // Setup process
    QProcess proc;
    proc.setEnvironment(arguments.environment.toStringList());
    proc.setWorkingDirectory(arguments.directory);
    proc.setProcessChannelMode(QProcess::MergedChannels);

    log->append(QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                          "Building helper '%1' in %2\n").arg(arguments.helperName,
                                                                              arguments.directory));
    log->append(newline);

    const QString makeFullPath = arguments.environment.searchInPath(arguments.makeCommand);
    if (QFileInfo(arguments.directory + QLatin1String("/Makefile")).exists()) {
        if (makeFullPath.isEmpty()) {
            *errorMessage = QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary",
                                                       "%1 not found in PATH\n").arg(arguments.makeCommand);
            return false;
        }
        const QString cleanTarget = QLatin1String("distclean");
        log->append(QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                                   "Running %1 %2...\n").arg(makeFullPath, cleanTarget));
        if (!runBuildProcess(proc, makeFullPath, QStringList(cleanTarget), 30000, true, log, errorMessage))
            return false;
    }
    QStringList qmakeArgs;
    if (!arguments.targetMode.isEmpty())
        qmakeArgs << arguments.targetMode;
    if (!arguments.mkspec.isEmpty())
        qmakeArgs << QLatin1String("-spec") << arguments.mkspec.toUserOutput();
    qmakeArgs << arguments.proFilename;
    qmakeArgs << arguments.qmakeArguments;

    log->append(newline);
    log->append(QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                            "Running %1 %2 ...\n").arg(arguments.qmakeCommand.toUserOutput(),
                                                                       qmakeArgs.join(QLatin1String(" "))));

    if (!runBuildProcess(proc, arguments.qmakeCommand.toString(), qmakeArgs, 30000, false, log, errorMessage))
        return false;
    log->append(newline);
    if (makeFullPath.isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                                "%1 not found in PATH\n").arg(arguments.makeCommand);
        return false;
    }
    log->append(QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                            "Running %1 %2 ...\n").arg(makeFullPath, arguments.makeArguments.join(QLatin1String(" "))));
    if (!runBuildProcess(proc, makeFullPath, arguments.makeArguments, 120000, false, log, errorMessage))
        return false;
    return true;
}

bool BuildableHelperLibrary::getHelperFileInfoFor(const QStringList &validBinaryFilenames,
                                                  const QString &directory, QFileInfo* info)
{
    if (!info)
        return false;

    foreach (const QString &binaryFilename, validBinaryFilenames) {
        info->setFile(directory + binaryFilename);
        if (info->exists())
            return true;
    }

    return false;
}

QString BuildableHelperLibrary::byInstallDataHelper(const QString &sourcePath,
                                                    const QStringList &sourceFileNames,
                                                    const QStringList &installDirectories,
                                                    const QStringList &validBinaryFilenames,
                                                    bool acceptOutdatedHelper)
{
    // find the latest change to the sources
    QDateTime sourcesModified;
    if (!acceptOutdatedHelper) {
        foreach (const QString &sourceFileName, sourceFileNames) {
            const QDateTime fileModified = QFileInfo(sourcePath + sourceFileName).lastModified();
            if (fileModified.isValid() && (!sourcesModified.isValid() || fileModified > sourcesModified))
                sourcesModified = fileModified;
        }
    }

    // We pretend that the lastmodified of dumper.cpp is 5 minutes before what
    // the file system says because afer a installation from the package the
    // modified dates of dumper.cpp and the actual library are close to each
    // other, but not deterministic in one direction.
    if (sourcesModified.isValid())
        sourcesModified = sourcesModified.addSecs(-300);

    // look for the newest helper library in the different locations
    QString newestHelper;
    QDateTime newestHelperModified = sourcesModified; // prevent using one that's older than the sources
    QFileInfo fileInfo;
    foreach (const QString &installDirectory, installDirectories) {
        if (getHelperFileInfoFor(validBinaryFilenames, installDirectory, &fileInfo)) {
            if (!newestHelperModified.isValid()
                    || (fileInfo.lastModified() > newestHelperModified)) {
                newestHelper = fileInfo.filePath();
                newestHelperModified = fileInfo.lastModified();
            }
        }
    }
    return newestHelper;
}

} // namespace Utils
