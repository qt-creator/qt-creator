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

#include "buildablehelperlibrary.h"
#include "hostosinfo.h"
#include "synchronousprocess.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QRegularExpression>

#include <set>

namespace Utils {

bool BuildableHelperLibrary::isQtChooser(const QFileInfo &info)
{
    return info.isSymLink() && info.symLinkTarget().endsWith(QLatin1String("/qtchooser"));
}

QString BuildableHelperLibrary::qtChooserToQmakePath(const QString &path)
{
    const QString toolDir = QLatin1String("QTTOOLDIR=\"");
    SynchronousProcess proc;
    proc.setTimeoutS(1);
    SynchronousProcessResponse response = proc.runBlocking({path, {"-print-env"}});
    if (response.result != SynchronousProcessResponse::Finished)
        return QString();
    const QString output = response.stdOut();
    int pos = output.indexOf(toolDir);
    if (pos == -1)
        return QString();
    pos += toolDir.count();
    int end = output.indexOf('\"', pos);
    if (end == -1)
        return QString();

    return output.mid(pos, end - pos) + QLatin1String("/qmake");
}

static bool isQmake(const QString &path)
{
    if (path.isEmpty())
        return false;
    QFileInfo fi(path);
    if (BuildableHelperLibrary::isQtChooser(fi))
        fi.setFile(BuildableHelperLibrary::qtChooserToQmakePath(fi.symLinkTarget()));
    if (!fi.exists() || fi.isDir())
        return false;
    return !BuildableHelperLibrary::qtVersionForQMake(fi.absoluteFilePath()).isEmpty();
}

static FilePath findQmakeInDir(const FilePath &path)
{
    if (path.isEmpty())
        return FilePath();

    const QString qmake = HostOsInfo::withExecutableSuffix("qmake");
    QDir dir(path.toString());
    if (dir.exists(qmake)) {
        const QString qmakePath = dir.absoluteFilePath(qmake);
        if (isQmake(qmakePath))
            return FilePath::fromString(qmakePath);
    }

    // Prefer qmake-qt5 to qmake-qt4 by sorting the filenames in reverse order.
    const QFileInfoList candidates = dir.entryInfoList(
                BuildableHelperLibrary::possibleQMakeCommands(),
                QDir::Files, QDir::Name | QDir::Reversed);
    for (const QFileInfo &fi : candidates) {
        if (fi.fileName() == qmake)
            continue;
        if (isQmake(fi.absoluteFilePath()))
            return FilePath::fromFileInfo(fi);
    }
    return FilePath();
}

FilePath BuildableHelperLibrary::findSystemQt(const Environment &env)
{
    const FilePaths list = findQtsInEnvironment(env, 1);
    return list.size() == 1 ? list.first() : FilePath();
}

FilePaths BuildableHelperLibrary::findQtsInEnvironment(const Environment &env, int maxCount)
{
    FilePaths qmakeList;
    std::set<QString> canonicalEnvPaths;
    const FilePaths paths = env.path();
    for (const FilePath &path : paths) {
        if (!canonicalEnvPaths.insert(path.toFileInfo().canonicalFilePath()).second)
            continue;
        const FilePath qmake = findQmakeInDir(path);
        if (qmake.isEmpty())
            continue;
        qmakeList << qmake;
        if (maxCount != -1 && qmakeList.size() == maxCount)
            break;
    }
    return qmakeList;
}

QString BuildableHelperLibrary::qtVersionForQMake(const QString &qmakePath)
{
    if (qmakePath.isEmpty())
        return QString();

    SynchronousProcess qmake;
    qmake.setTimeoutS(5);
    SynchronousProcessResponse response = qmake.runBlocking({qmakePath, {"--version"}});
    if (response.result != SynchronousProcessResponse::Finished) {
        qWarning() << response.exitMessage(qmakePath, 5);
        return QString();
    }

    const QString output = response.allOutput();
    static const QRegularExpression regexp("(QMake version:?)[\\s]*([\\d.]*)",
                                           QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = regexp.match(output);
    const QString qmakeVersion = match.captured(2);
    if (qmakeVersion.startsWith(QLatin1String("2."))
            || qmakeVersion.startsWith(QLatin1String("3."))) {
        static const QRegularExpression regexp2("Using Qt version[\\s]*([\\d\\.]*)",
                                                QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch match2 = regexp2.match(output);
        const QString version = match2.captured(1);
        return version;
    }
    return QString();
}

QString BuildableHelperLibrary::filterForQmakeFileDialog()
{
    QString filter = QLatin1String("qmake (");
    const QStringList commands = possibleQMakeCommands();
    for (int i = 0; i < commands.size(); ++i) {
        if (i)
            filter += QLatin1Char(' ');
        if (HostOsInfo::isMacHost())
            // work around QTBUG-7739 that prohibits filters that don't start with *
            filter += QLatin1Char('*');
        filter += commands.at(i);
        if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost())
            // kde bug, we need at least one wildcard character
            // see QTCREATORBUG-7771
            filter += QLatin1Char('*');
    }
    filter += QLatin1Char(')');
    return filter;
}


QStringList BuildableHelperLibrary::possibleQMakeCommands()
{
    // On Windows it is always "qmake.exe"
    // On Unix some distributions renamed qmake with a postfix to avoid clashes
    // On OS X, Qt 4 binary packages also has renamed qmake. There are also symbolic links that are
    // named "qmake", but the file dialog always checks against resolved links (native Cocoa issue)
    return QStringList(HostOsInfo::withExecutableSuffix("qmake*"));
}

// Copy helper source files to a target directory, replacing older files.
bool BuildableHelperLibrary::copyFiles(const QString &sourcePath,
                                     const QStringList &files,
                                     const QString &targetDirectory,
                                     QString *errorMessage)
{
    // try remove the directory
    if (!FileUtils::removeRecursively(FilePath::fromString(targetDirectory), errorMessage))
        return false;
    if (!QDir().mkpath(targetDirectory)) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary", "The target directory %1 could not be created.").arg(targetDirectory);
        return false;
    }
    for (const QString &file : files) {
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
                                    const FilePath &binary,
                                    const QStringList &args,
                                    int timeoutS,
                                    bool ignoreNonNullExitCode,
                                    QString *output, QString *errorMessage)
{
    proc.start(binary.toString(), args);
    if (!proc.waitForStarted()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                                    "Cannot start process: %1").
                                                    arg(proc.errorString());
        return false;
    }
    // Read stdout/err and check for timeouts
    QByteArray stdOut;
    QByteArray stdErr;
    if (!SynchronousProcess::readDataFromProcess(proc, timeoutS, &stdOut, &stdErr, false)) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                                    "Timeout after %1 s.").
                                                    arg(timeoutS);
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
                            const FilePath &binary,
                            const QStringList &args,
                            int timeoutS,
                            bool ignoreNonNullExitCode,
                            QString *output, QString *errorMessage)
{
    const bool rc = runBuildProcessI(proc, binary, args, timeoutS, ignoreNonNullExitCode, output, errorMessage);
    if (!rc) {
        // Fail - reformat error.
        QString cmd = binary.toString();
        if (!args.isEmpty()) {
            cmd += QLatin1Char(' ');
            cmd += args.join(QLatin1Char(' '));
        }
        *errorMessage =
                QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                            "Error running \"%1\" in %2: %3").
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
                                          "Building helper \"%1\" in %2\n").arg(arguments.helperName,
                                                                              arguments.directory));
    log->append(newline);

    const FilePath makeFullPath = arguments.environment.searchInPath(arguments.makeCommand);
    if (QFileInfo::exists(arguments.directory + QLatin1String("/Makefile"))) {
        if (makeFullPath.isEmpty()) {
            *errorMessage = QCoreApplication::translate("ProjectExplorer::DebuggingHelperLibrary",
                                                       "%1 not found in PATH\n").arg(arguments.makeCommand);
            return false;
        }
        const QString cleanTarget = QLatin1String("distclean");
        log->append(QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                                   "Running %1 %2...\n")
                    .arg(makeFullPath.toUserOutput(), cleanTarget));
        if (!runBuildProcess(proc, makeFullPath, QStringList(cleanTarget), 30, true, log, errorMessage))
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
                                                                       qmakeArgs.join(QLatin1Char(' '))));

    if (!runBuildProcess(proc, arguments.qmakeCommand, qmakeArgs, 30, false, log, errorMessage))
        return false;
    log->append(newline);
    if (makeFullPath.isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                                "%1 not found in PATH\n").arg(arguments.makeCommand);
        return false;
    }
    log->append(QCoreApplication::translate("ProjectExplorer::BuildableHelperLibrary",
                                            "Running %1 %2 ...\n")
                .arg(makeFullPath.toUserOutput(), arguments.makeArguments.join(QLatin1Char(' '))));
    return runBuildProcess(proc, makeFullPath, arguments.makeArguments, 120, false, log, errorMessage);
}

bool BuildableHelperLibrary::getHelperFileInfoFor(const QStringList &validBinaryFilenames,
                                                  const QString &directory, QFileInfo* info)
{
    if (!info)
        return false;

    for (const QString &binaryFilename : validBinaryFilenames) {
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
        for (const QString &sourceFileName : sourceFileNames) {
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
    for (const QString &installDirectory : installDirectories) {
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
