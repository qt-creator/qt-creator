// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pipsupport.h"

#include "pythonplugin.h"
#include "pythontr.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/mimeutils.h>
#include <utils/process.h>

using namespace Utils;

namespace Python::Internal {

const char pipInstallTaskId[] = "Python::pipInstallTask";

PipInstallTask::PipInstallTask(const FilePath &python)
    : m_python(python)
{
    connect(&m_process, &Process::done, this, &PipInstallTask::handleDone);
    connect(&m_process, &Process::readyReadStandardError, this, &PipInstallTask::handleError);
    connect(&m_process, &Process::readyReadStandardOutput, this, &PipInstallTask::handleOutput);
    connect(&m_killTimer, &QTimer::timeout, this, &PipInstallTask::cancel);
    connect(&m_watcher, &QFutureWatcher<void>::canceled, this, &PipInstallTask::cancel);
    m_watcher.setFuture(m_future.future());
}

void PipInstallTask::setRequirements(const Utils::FilePath &requirementFile)
{
    m_requirementsFile = requirementFile;
}

void PipInstallTask::setWorkingDirectory(const Utils::FilePath &workingDirectory)
{
    m_process.setWorkingDirectory(workingDirectory);
}

void PipInstallTask::addPackage(const PipPackage &package)
{
    m_packages << package;
}

void PipInstallTask::setPackages(const QList<PipPackage> &packages)
{
    m_packages = packages;
}

void PipInstallTask::run()
{
    if (m_packages.isEmpty() && m_requirementsFile.isEmpty()) {
        emit finished(false);
        return;
    }
    const QString taskTitle = Tr::tr("Install Python Packages");
    Core::ProgressManager::addTask(m_future.future(), taskTitle, pipInstallTaskId);
    QStringList arguments = {"-m", "pip", "install"};
    if (!m_requirementsFile.isEmpty())
        arguments << "-r" << m_requirementsFile.toString();
    else {
        for (const PipPackage &package : m_packages) {
            QString pipPackage = package.packageName;
            if (!package.version.isEmpty())
                pipPackage += "==" + package.version;
            arguments << pipPackage;
        }
    }

    // add --user to global pythons, but skip it for venv pythons
    if (!QDir(m_python.parentDir().toString()).exists("activate"))
        arguments << "--user";

    m_process.setCommand({m_python, arguments});
    m_process.setTerminalMode(TerminalMode::Run);
    m_process.start();

    Core::MessageManager::writeSilently(
        Tr::tr("Running \"%1\" to install %2.")
            .arg(m_process.commandLine().toUserOutput(), packagesDisplayName()));

    m_killTimer.setSingleShot(true);
    m_killTimer.start(5 /*minutes*/ * 60 * 1000);
}

void PipInstallTask::cancel()
{
    m_process.stop();
    m_process.waitForFinished();
    Core::MessageManager::writeFlashing(
        m_killTimer.isActive()
            ? Tr::tr("The installation of \"%1\" was canceled by timeout.").arg(packagesDisplayName())
            : Tr::tr("The installation of \"%1\" was canceled by the user.")
                  .arg(packagesDisplayName()));
}

void PipInstallTask::handleDone()
{
    m_future.reportFinished();
    const bool success = m_process.result() == ProcessResult::FinishedWithSuccess;
    if (!success) {
        Core::MessageManager::writeFlashing(Tr::tr("Installing \"%1\" failed with exit code %2.")
                                                .arg(packagesDisplayName())
                                                .arg(m_process.exitCode()));
    }
    emit finished(success);
}

void PipInstallTask::handleOutput()
{
    const QString &stdOut = QString::fromLocal8Bit(m_process.readAllRawStandardOutput().trimmed());
    if (!stdOut.isEmpty())
        Core::MessageManager::writeSilently(stdOut);
}

void PipInstallTask::handleError()
{
    const QString &stdErr = QString::fromLocal8Bit(m_process.readAllRawStandardError().trimmed());
    if (!stdErr.isEmpty())
        Core::MessageManager::writeSilently(stdErr);
}

QString PipInstallTask::packagesDisplayName() const
{
    return m_requirementsFile.isEmpty()
               ? Utils::transform(m_packages, &PipPackage::displayName).join(", ")
               : m_requirementsFile.toUserOutput();
}

void PipPackageInfo::parseField(const QString &field, const QStringList &data)
{
    if (field.isEmpty())
        return;
    if (field == "Name") {
        name = data.value(0);
    } else if (field == "Version") {
        version = data.value(0);
    } else if (field == "Summary") {
        summary = data.value(0);
    } else if (field == "Home-page") {
        homePage = QUrl(data.value(0));
    } else if (field == "Author") {
        author = data.value(0);
    } else if (field == "Author-email") {
        authorEmail = data.value(0);
    } else if (field == "License") {
        license = data.value(0);
    } else if (field == "Location") {
        location = FilePath::fromUserInput(data.value(0)).normalizedPathName();
    } else if (field == "Requires") {
        requiresPackage = data.value(0).split(',', Qt::SkipEmptyParts);
    } else if (field == "Required-by") {
        requiredByPackage = data.value(0).split(',', Qt::SkipEmptyParts);
    } else if (field == "Files") {
        for (const QString &fileName : data) {
            if (!fileName.isEmpty())
                files.append(FilePath::fromUserInput(fileName.trimmed()));
        }
    }
}

Pip *Pip::instance(const FilePath &python)
{
    static QMap<FilePath, Pip *> pips;
    auto it = pips.find(python);
    if (it == pips.end())
        it = pips.insert(python, new Pip(python));
    return it.value();
}

static PipPackageInfo infoImpl(const PipPackage &package, const FilePath &python)
{
    PipPackageInfo result;

    Process pip;
    pip.setCommand(CommandLine(python, {"-m", "pip", "show", "-f", package.packageName}));
    pip.runBlocking();
    QString fieldName;
    QStringList data;
    const QString pipOutput = pip.allOutput();
    for (const QString &line : pipOutput.split('\n')) {
        if (line.isEmpty())
            continue;
        if (line.front().isSpace()) {
            data.append(line.trimmed());
        } else {
            result.parseField(fieldName, data);
            if (auto colonPos = line.indexOf(':'); colonPos >= 0) {
                fieldName = line.left(colonPos);
                data = QStringList(line.mid(colonPos + 1).trimmed());
            } else {
                fieldName.clear();
                data.clear();
            }
        }
    }
    result.parseField(fieldName, data);
    return result;
}

QFuture<PipPackageInfo> Pip::info(const PipPackage &package)
{
    return Utils::asyncRun(infoImpl, package, m_python);
}

Pip::Pip(const Utils::FilePath &python)
    : QObject(PythonPlugin::instance())
    , m_python(python)
{}

} // Python::Internal
