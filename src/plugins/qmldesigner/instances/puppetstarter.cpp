// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "puppetstarter.h"

#include <qmldesignertr.h>

#include <QCoreApplication>
#include <QMessageBox>
#include <QProcess>

namespace QmlDesigner::PuppetStarter {

using NanotraceHR::keyValue;
using NodeInstanceTracing::category;

QProcessUniquePointer createPuppetProcess(const PuppetStartData &data,
                                          const QString &puppetMode,
                                          const QString &socketToken,
                                          std::function<void()> processOutputCallback,
                                          std::function<void(int, QProcess::ExitStatus)> processFinishCallback,
                                          const QStringList &customOptions)
{
    NanotraceHR::Tracer tracer{"puppet starter create puppet process",
                               category(),
                               keyValue("path", data.puppetPath),
                               keyValue("working directory", data.workingDirectoryPath),
                               keyValue("forward output", data.forwardOutput),
                               keyValue("dump environment", data.dumpInitInformation),
                               keyValue("free type option", data.freeTypeOption),
                               keyValue("debug puppet", data.debugPuppet),
                               keyValue("puppet mode", puppetMode),
                               keyValue("socket token", socketToken)};

    QProcessUniquePointer puppetProcess{new QProcess};
    puppetProcess->setObjectName(puppetMode);
    puppetProcess->setProcessEnvironment(data.environment);

    QObject::connect(QCoreApplication::instance(),
                     &QCoreApplication::aboutToQuit,
                     puppetProcess.get(),
                     &QProcess::kill);
    QObject::connect(puppetProcess.get(), &QProcess::finished, processFinishCallback);

    puppetProcess->setWorkingDirectory(data.workingDirectoryPath);

    QStringList processArguments;
    if (puppetMode != "custom")
        processArguments = {socketToken, puppetMode};

    processArguments.append(customOptions);
    processArguments.push_back(data.freeTypeOption);

    if (data.forwardOutput == puppetMode || data.forwardOutput == "all") {
        if (data.dumpInitInformation) {
            qDebug().noquote() << QString("%1 arguments:\n%2").arg(puppetMode, processArguments.join(" "));

            QStringList envList;
            for (const QString &key : data.environment.keys())
                envList.append(key + "=" + data.environment.value(key));
            envList.sort();
            qDebug().noquote() << QString("%1 environment:\n%2").arg(puppetMode, envList.join("\n"));
        }
        puppetProcess->setProcessChannelMode(QProcess::MergedChannels);
        QObject::connect(puppetProcess.get(), &QProcess::readyRead, processOutputCallback);
    }
    puppetProcess->start(data.puppetPath, processArguments);

    if (data.debugPuppet == puppetMode || data.debugPuppet == "all") {
        QMessageBox::information(
            nullptr,
            Tr::tr("Puppet is starting..."),
            Tr::tr("You can now attach your debugger to the %1(%2) QML Puppet with process id: %3.")
                .arg(puppetMode, data.puppetPath, QString::number(puppetProcess->processId())));
    }

    return puppetProcess;
}

} // namespace QmlDesigner::PuppetStarter
