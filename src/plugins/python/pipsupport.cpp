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

#include <solutions/tasking/tasktree.h>

#include <utils/algorithm.h>
#include <utils/mimeutils.h>
#include <utils/qtcprocess.h>

using namespace Core;
using namespace Tasking;
using namespace Utils;

namespace Python::Internal {

QString PipInstallerData::packagesDisplayName() const
{
    return requirementsFile.isEmpty()
               ? Utils::transform(packages, &PipPackage::displayName).join(", ")
               : requirementsFile.toUserOutput();
}

Group pipInstallerTask(const PipInstallerData &data)
{
    const auto onSetup = [data](Process &process) {
        if (data.packages.isEmpty() && data.requirementsFile.isEmpty())
            return SetupResult::StopWithError;

        QStringList arguments = {"-m", "pip", "install"};
        if (!data.requirementsFile.isEmpty()) {
            arguments << "-r" << data.requirementsFile.toUrlishString();
        } else {
            for (const PipPackage &package : std::as_const(data.packages)) {
                QString pipPackage = package.packageName;
                if (!package.version.isEmpty())
                    pipPackage += "==" + package.version;
                arguments << pipPackage;
            }
        }

        if (!data.targetPath.isEmpty()) {
            QTC_ASSERT(data.targetPath.isSameDevice(data.python), return SetupResult::StopWithError);
            arguments << "-t" << data.targetPath.path();
        } else if (!isVenvPython(data.python)) {
            arguments << "--user"; // add --user to global pythons, but skip it for venv pythons
        }

        if (data.upgrade)
            arguments << "--upgrade";

        QString operation;
        if (!data.requirementsFile.isEmpty()) {
            operation = data.upgrade ? Tr::tr("Update Requirements") : Tr::tr("Install Requirements");
        } else if (data.packages.count() == 1) {
            //: %1 = package name
            operation = data.upgrade ? Tr::tr("Update %1")
                                       //: %1 = package name
                                       : Tr::tr("Install %1");
            operation = operation.arg(data.packages.first().displayName);
        } else {
            operation = data.upgrade ? Tr::tr("Update Packages") : Tr::tr("Install Packages");
        }

        process.setCommand({data.python, arguments});
        process.setTerminalMode(data.silent ? TerminalMode::Off : TerminalMode::Run);
        auto progress = new ProcessProgress(&process);
        progress->setDisplayName(operation);

        MessageManager::writeSilently(Tr::tr("Running \"%1\" to install %2.")
                .arg(process.commandLine().toUserOutput(), data.packagesDisplayName()));

        QObject::connect(&process, &Process::readyReadStandardError, &process, [process = &process] {
            const QString &stdOut = QString::fromLocal8Bit(process->readAllRawStandardOutput().trimmed());
            if (!stdOut.isEmpty())
                MessageManager::writeSilently(stdOut);
        });
        QObject::connect(&process, &Process::readyReadStandardOutput, &process, [process = &process] {
            const QString &stdErr = QString::fromLocal8Bit(process->readAllRawStandardError().trimmed());
            if (!stdErr.isEmpty())
                MessageManager::writeSilently(stdErr);
        });
        return SetupResult::Continue;
    };

    const auto packagesDisplayName = data.packagesDisplayName();

    const auto onDone = [packagesDisplayName](const Process &process) {
        MessageManager::writeFlashing(Tr::tr("Installing \"%1\" failed: %2")
                                          .arg(packagesDisplayName, process.exitMessage()));
    };

    const auto onTimeout = [packagesDisplayName] {
        MessageManager::writeFlashing(
            Tr::tr("The installation of \"%1\" was canceled by timeout.").arg(packagesDisplayName));
    };

    using namespace std::literals::chrono_literals;
    return {
        ProcessTask(onSetup, onDone, CallDone::OnError)
            .withTimeout(5min, onTimeout)
    };
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
