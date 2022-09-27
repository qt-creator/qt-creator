// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "puppetstarter.h"

#include <QCoreApplication>
#include <QMessageBox>
#include <QProcess>

namespace QmlDesigner {

PuppetStarter::PuppetStarter(ProjectExplorer::Target *target, const Model *model)

    : m_target(target)
    , m_availablePuppetType(FallbackPuppet)
    , m_model(model)
{
}

QProcessUniquePointer PuppetStarter::createPuppetProcess(
    const PuppetStartData &data,
    const QString &puppetMode,
    const QString &socketToken,
    std::function<void()> processOutputCallback,
    std::function<void(int, QProcess::ExitStatus)> processFinishCallback,
    const QStringList &customOptions) const
{
    return puppetProcess(data.puppetPath,
                         data.workingDirectoryPath,
                         data.forwardOutput,
                         data.freeTypeOption,
                         data.debugPuppet,
                         data.environment,
                         puppetMode,
                         socketToken,
                         processOutputCallback,
                         processFinishCallback,
                         customOptions);
}

QProcessUniquePointer PuppetStarter::puppetProcess(
    const QString &puppetPath,
    const QString &workingDirectory,
    const QString &forwardOutput,
    const QString &freeTypeOption,
    const QString &debugPuppet,
    const QProcessEnvironment &processEnvironment,
    const QString &puppetMode,
    const QString &socketToken,
    std::function<void()> processOutputCallback,
    std::function<void(int, QProcess::ExitStatus)> processFinishCallback,
    const QStringList &customOptions) const
{
    QProcessUniquePointer puppetProcess{new QProcess};
    puppetProcess->setObjectName(puppetMode);
    puppetProcess->setProcessEnvironment(processEnvironment);

    QObject::connect(QCoreApplication::instance(),
                     &QCoreApplication::aboutToQuit,
                     puppetProcess.get(),
                     &QProcess::kill);
    QObject::connect(puppetProcess.get(),
                     static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                     processFinishCallback);

    if (forwardOutput == puppetMode || forwardOutput == "all") {
        puppetProcess->setProcessChannelMode(QProcess::MergedChannels);
        QObject::connect(puppetProcess.get(), &QProcess::readyRead, processOutputCallback);
    }
    puppetProcess->setWorkingDirectory(workingDirectory);

    QStringList processArguments;
    if (puppetMode == "custom")
        processArguments = customOptions;
    else
        processArguments = {socketToken, puppetMode};

    processArguments.push_back("-graphicssystem raster");
    processArguments.push_back(freeTypeOption);

    puppetProcess->start(puppetPath, processArguments);

    if (debugPuppet == puppetMode || debugPuppet == "all") {
        QMessageBox::information(
            nullptr,
            QCoreApplication::translate("PuppetStarter", "Puppet is starting..."),
            QCoreApplication::translate(
                "PuppetStarter",
                "You can now attach your debugger to the %1 puppet with process id: %2.")
                .arg(puppetMode, QString::number(puppetProcess->processId())));
    }

    return puppetProcess;
}

} // namespace QmlDesigner
