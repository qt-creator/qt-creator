// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpfilesystemhandler.h"
#include "acpclientobject.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/textdocument.h>

#include <utils/filepath.h>
#include <utils/result.h>
#include <utils/textfileformat.h>

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

    QString text;
    if (auto *doc = qobject_cast<Core::BaseTextDocument *>(Core::DocumentModel::documentForFilePath(filePath))) {
        text = doc->plainText();
    } else {
        TextFileFormat::ReadResult result
            = TextFileFormat().readFile(filePath, TextEncoding::encodingForLocale());
        if (result.code != TextFileFormat::ReadSuccess) {
            m_client->sendErrorResponse(
                id,
                ErrorCode::Internal_error,
                QStringLiteral("Cannot read file: %1. %2")
                    .arg(filePath.toUserOutput())
                    .arg(result.error));
            return;
        }

        text = result.content;
    }

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

    // try to detect file encoding by reading the file contents first, if it exists
    TextFileFormat format;
    if (const Result<QByteArray> contents = filePath.fileContents(); contents)
        format.detectFromData(*contents);
    if (!format.encoding().isValid())
        format.setEncoding(TextEncoding::encodingForLocale());

    if (format.writeFile(filePath, request.content())) {
        WriteTextFileResponse response;
        m_client->sendResponse(id, Acp::toJson(response));
    }
    m_client->sendErrorResponse(id, ErrorCode::Internal_error,
                                QStringLiteral("Cannot write file: %1").arg(filePath.toUserOutput()));
    return;

}

} // namespace AcpClient::Internal
