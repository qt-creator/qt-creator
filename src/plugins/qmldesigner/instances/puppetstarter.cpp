// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "puppetstarter.h"
#include "nodeinstancetracing.h"

#include <qmldesignertr.h>

#include <QCoreApplication>
#include <QMessageBox>
#include <QProcess>

namespace QmlDesigner::PuppetStarter {

using NanotraceHR::keyValue;
using NodeInstanceTracing::category;

namespace {
QProcessUniquePointer puppetProcess(const QString &puppetPath,
                                    const QString &workingDirectory,
                                    const QString &forwardOutput,
                                    const QString &freeTypeOption,
                                    const QString &debugPuppet,
                                    const QProcessEnvironment &processEnvironment,
                                    const QString &puppetMode,
                                    const QString &socketToken,
                                    std::function<void()> processOutputCallback,
                                    std::function<void(int, QProcess::ExitStatus)> processFinishCallback,
                                    const QStringList &customOptions)
{
    NanotraceHR::Tracer tracer{"puppet starter puppet process",
                               category(),
                               keyValue("path", puppetPath),
                               keyValue("working directory", workingDirectory),
                               keyValue("forward output", forwardOutput),
                               keyValue("free type option", freeTypeOption),
                               keyValue("debug puppet", debugPuppet),
                               keyValue("puppet mode", puppetMode),
                               keyValue("socket token", socketToken)};

    QProcessUniquePointer puppetProcess{new QProcess};
    puppetProcess->setObjectName(puppetMode);
    puppetProcess->setProcessEnvironment(processEnvironment);

    QObject::connect(QCoreApplication::instance(),
                     &QCoreApplication::aboutToQuit,
                     puppetProcess.get(),
                     &QProcess::kill);
    QObject::connect(puppetProcess.get(),
                     &QProcess::finished,
                     processFinishCallback);

    if (forwardOutput == puppetMode || forwardOutput == "all") {
        puppetProcess->setProcessChannelMode(QProcess::MergedChannels);
        QObject::connect(puppetProcess.get(), &QProcess::readyRead, processOutputCallback);
    }
    puppetProcess->setWorkingDirectory(workingDirectory);

    QStringList processArguments;
    if (puppetMode != "custom")
        processArguments = {socketToken, puppetMode};

    processArguments.append(customOptions);
    processArguments.push_back(freeTypeOption);

    puppetProcess->start(puppetPath, processArguments);

    if (debugPuppet == puppetMode || debugPuppet == "all") {
        QMessageBox::information(
            nullptr,
            Tr::tr("Puppet is starting..."),
            Tr::tr("You can now attach your debugger to the %1(%2) QML Puppet with process id: %3.")
                .arg(puppetMode, puppetPath, QString::number(puppetProcess->processId())));
    }

    return puppetProcess;
}

} // namespace

QProcessUniquePointer createPuppetProcess(const PuppetStartData &data,
                                          const QString &puppetMode,
                                          const QString &socketToken,
                                          std::function<void()> processOutputCallback,
                                          std::function<void(int, QProcess::ExitStatus)> processFinishCallback,
                                          const QStringList &customOptions)
{
    NanotraceHR::Tracer tracer{"puppet starter create puppet process", category()};

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

} // namespace QmlDesigner::PuppetStarter
