// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpfilesystemhandler.h"
#include "acpclientobject.h"

#include <utils/filepath.h>
#include <utils/result.h>

#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(logFs, "qtc.acpclient.filesystem", QtWarningMsg);

using namespace Acp;
using namespace Utils;

namespace AcpClient::Internal {

AcpFilesystemHandler::AcpFilesystemHandler(AcpClientObject *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    connect(client, &AcpClientObject::readTextFileRequested,
            this, &AcpFilesystemHandler::handleReadTextFile);
    connect(client, &AcpClientObject::writeTextFileRequested,
            this, &AcpFilesystemHandler::handleWriteTextFile);
}

void AcpFilesystemHandler::handleReadTextFile(const QJsonValue &id, const ReadTextFileRequest &request)
{
    const FilePath filePath = FilePath::fromUserInput(request.path());
    qCDebug(logFs) << "Reading file:" << filePath;

    const Result<QByteArray> contents = filePath.fileContents();
    if (!contents) {
        m_client->sendErrorResponse(id, ErrorCode::Resource_not_found,
                                    QStringLiteral("Cannot read file: %1").arg(filePath.toUserOutput()));
        return;
    }

    QString text = QString::fromUtf8(*contents);

    // Apply line offset and limit if specified
    const auto startLine = request.line();
    const auto lineLimit = request.limit();

    if (startLine || lineLimit) {
        const QStringList lines = text.split('\n');
        const int start = startLine.value_or(0);
        const int count = lineLimit.value_or(lines.size() - start);
        const QStringList subset = lines.mid(start, count);
        text = subset.join('\n');
    }

    ReadTextFileResponse response;
    response.content(text);
    m_client->sendResponse(id, Acp::toJson(response));
}

void AcpFilesystemHandler::handleWriteTextFile(const QJsonValue &id, const WriteTextFileRequest &request)
{
    const FilePath filePath = FilePath::fromUserInput(request.path());
    qCDebug(logFs) << "Writing file:" << filePath;

    const Result<qint64> result = filePath.writeFileContents(request.content().toUtf8());
    if (!result) {
        m_client->sendErrorResponse(id, ErrorCode::Internal_error,
                                    QStringLiteral("Cannot write file: %1").arg(filePath.toUserOutput()));
        return;
    }

    WriteTextFileResponse response;
    m_client->sendResponse(id, Acp::toJson(response));
}

} // namespace AcpClient::Internal
