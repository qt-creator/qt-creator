// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltraceviewerrpc.h"

#include "qmltraceviewersettings.h"
#include "schema/api.h"

#include <utils/filepath.h>

#include <iostream>

using namespace QmlTraceViewer::Api::Schema;

namespace QmlTraceViewer::RPC {

static void writeQJsonObject(const QJsonObject &object)
{
    if (!settings().withNotifications())
        return;

    const QJsonDocument document(object);
    const QByteArray encoded = document.toJson(QJsonDocument::Compact);
    std::cout << encoded.toStdString() << std::endl;
}

void notifyTraceFileLoadingStarted(const Utils::FilePath &file)
{
    const TraceFileLoadingStartedNotification notification(
        TraceFileLoadingStartedNotification::Params()
            .traceFilePath(file.toUserOutput()));

    writeQJsonObject(toJson(notification));
}

void notifyTraceFileLoadingFinished(const Utils::FilePath &file, const QString &errorMessage)
{
    const TraceFileLoadingFinishedNotification notification(
        TraceFileLoadingFinishedNotification::Params()
            .traceFilePath(file.toUserOutput())
            .successful(errorMessage.isEmpty())
            .errorMessage(errorMessage));

    writeQJsonObject(toJson(notification));
}

void notifyTraceEventSelected(const QString &sourceFilePath, int line, int column)
{
    const TraceEventSelectedNotification notification(
        TraceEventSelectedNotification::Params()
            .sourceFilePath(sourceFilePath)
            .lineNumber(line)
            .columnNumber(column));

    writeQJsonObject(toJson(notification));
}

void notifyTraceDiscarded()
{
    const TraceDiscardedNotification notification;
    writeQJsonObject(toJson(notification));
}

} // namespace QmlTraceViewer::RPC
