// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pipsupport.h"

#include "pythontr.h"
#include "pythonutils.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/processprogress.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/mimeutils.h>
#include <utils/qtcprocess.h>

using namespace Utils;

namespace Python::Internal {

PipInstallTask::PipInstallTask(const FilePath &python)
    : m_python(python)
{
    connect(&m_process, &Process::done, this, &PipInstallTask::handleDone);
    connect(&m_process, &Process::readyReadStandardError, this, &PipInstallTask::handleError);
    connect(&m_process, &Process::readyReadStandardOutput, this, &PipInstallTask::handleOutput);
    connect(&m_killTimer, &QTimer::timeout, this, &PipInstallTask::cancel);
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

void PipInstallTask::setTargetPath(const Utils::FilePath &targetPath)
{
    m_targetPath = targetPath;
}

void PipInstallTask::run()
{
    if (m_packages.isEmpty() && m_requirementsFile.isEmpty()) {
        emit finished(false);
        return;
    }
    QStringList arguments = {"-m", "pip", "install"};
    if (!m_requirementsFile.isEmpty()) {
        arguments << "-r" << m_requirementsFile.toUrlishString();
    } else {
        for (const PipPackage &package : std::as_const(m_packages)) {
            QString pipPackage = package.packageName;
            if (!package.version.isEmpty())
                pipPackage += "==" + package.version;
            arguments << pipPackage;
        }
    }

    if (!m_targetPath.isEmpty()) {
        QTC_ASSERT(m_targetPath.isSameDevice(m_python), emit finished(false); return);
        arguments << "-t" << m_targetPath.path();
    } else if (!isVenvPython(m_python)) {
        arguments << "--user"; // add --user to global pythons, but skip it for venv pythons
    }

    if (m_upgrade)
        arguments << "--upgrade";

    QString operation;
    if (!m_requirementsFile.isEmpty()) {
        operation = m_upgrade ? Tr::tr("Update Requirements") : Tr::tr("Install Requirements");
    } else if (m_packages.count() == 1) {
        //: %1 = package name
        operation = m_upgrade ? Tr::tr("Update %1")
                              //: %1 = package name
                              : Tr::tr("Install %1");
        operation = operation.arg(m_packages.first().displayName);
    } else {
        operation = m_upgrade ? Tr::tr("Update Packages") : Tr::tr("Install Packages");
    }

    m_process.setCommand({m_python, arguments});
    m_process.setTerminalMode(m_silent ? TerminalMode::Off : TerminalMode::Run);
    auto progress = new Core::ProcessProgress(&m_process);
    progress->setDisplayName(operation);
    m_process.start();

    Core::MessageManager::writeSilently(
        Tr::tr("Running \"%1\" to install %2.")
            .arg(m_process.commandLine().toUserOutput(), packagesDisplayName()));

    m_killTimer.setSingleShot(true);
    m_killTimer.start(5 /*minutes*/ * 60 * 1000);
}

void PipInstallTask::cancel()
{
    m_process.close();
    Core::MessageManager::writeFlashing(
        Tr::tr("The installation of \"%1\" was canceled by timeout.").arg(packagesDisplayName()));
}

void PipInstallTask::handleDone()
{
    m_killTimer.stop();
    const bool success = m_process.result() == ProcessResult::FinishedWithSuccess;
    if (!success) {
        Core::MessageManager::writeFlashing(Tr::tr("Installing \"%1\" failed: %2")
                             .arg(packagesDisplayName(), m_process.exitMessage()));
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

void PipInstallTask::setUpgrade(bool upgrade)
{
    m_upgrade = upgrade;
}

void PipInstallTask::setSilent(bool silent)
{
    m_silent = silent;
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

} // Python::Internal
