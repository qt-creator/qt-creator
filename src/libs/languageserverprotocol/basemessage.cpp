// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "basemessage.h"

#include "jsonrpcmessages.h"
#include "languageserverprotocoltr.h"

#include <QBuffer>

#include <cstring>
#include <utility>

using namespace Utils;

namespace LanguageServerProtocol {

Q_LOGGING_CATEGORY(parseLog, "qtc.languageserverprotocol.parse", QtWarningMsg)

constexpr char headerFieldSeparator[] = ": ";
constexpr char contentCharsetName[] = "charset";
constexpr char defaultCharset[] = "utf-8";
constexpr char contentLengthFieldName[] = "Content-Length";
constexpr char headerSeparator[] = "\r\n";
constexpr char contentTypeFieldName[] = "Content-Type";

BaseMessage::BaseMessage()
    : mimeType(JsonRpcMessage::jsonRpcMimeType())
{ }

BaseMessage::BaseMessage(const QByteArray &mimeType, const QByteArray &content,
                         int expectedLength, const TextEncoding &encoding)
    : mimeType(mimeType.isEmpty() ? JsonRpcMessage::jsonRpcMimeType() : mimeType)
    , content(content)
    , contentLength(expectedLength)
    , encoding(encoding)
{ }

BaseMessage::BaseMessage(const QByteArray &mimeType, const QByteArray &content)
    : BaseMessage(mimeType, content, content.length(), defaultEncoding())
{ }

bool BaseMessage::operator==(const BaseMessage &other) const
{
    return mimeType == other.mimeType && content == other.content && encoding == other.encoding;
}

static QPair<QByteArray, QByteArray> splitHeaderFieldLine(const QByteArray &headerFieldLine)
{
    static const int fieldSeparatorLength = int(std::strlen(headerFieldSeparator));
    int assignmentIndex = headerFieldLine.indexOf(headerFieldSeparator);
    if (assignmentIndex >= 0) {
        return {headerFieldLine.mid(0, assignmentIndex),
                    headerFieldLine.mid(assignmentIndex + fieldSeparatorLength)};
    }
    qCWarning(parseLog) << "Unexpected header line:" << QLatin1String(headerFieldLine);
    return {};
}

static void parseContentType(BaseMessage &message, QByteArray contentType, QString &parseError)
{
    if (contentType.startsWith('"') && contentType.endsWith('"'))
        contentType = contentType.mid(1, contentType.length() - 2);
    QList<QByteArray> contentTypeElements = contentType.split(';');
    QByteArray mimeTypeName = contentTypeElements.takeFirst();
    TextEncoding encoding;
    for (const QByteArray &_contentTypeElement : std::as_const(contentTypeElements)) {
        const QByteArray &contentTypeElement = _contentTypeElement.trimmed();
        if (contentTypeElement.startsWith(contentCharsetName)) {
            const int equalindex = contentTypeElement.indexOf('=');
            const QByteArray charset = contentTypeElement.mid(equalindex + 1);
            if (equalindex > 0)
                encoding = TextEncoding(charset);
            if (!encoding.isValid()) {
                parseError = Tr::tr("Cannot decode content with \"%1\". Falling back to \"%2\".")
                                 .arg(QLatin1String(charset),
                                      QLatin1String(defaultCharset));
            }
        }
    }
    message.mimeType = mimeTypeName;
    message.encoding = encoding.isValid() ? encoding : BaseMessage::defaultEncoding();
}

static void parseContentLength(BaseMessage &message, QByteArray contentLength, QString &parseError)
{
    bool ok = true;
    message.contentLength = contentLength.toInt(&ok);
    if (!ok) {
        parseError = Tr::tr("Expected an integer in \"%1\", but got \"%2\".")
                .arg(QString::fromLatin1(contentLengthFieldName), QString::fromLatin1(contentLength));
    }
}

void BaseMessage::parse(QBuffer *data, QString &parseError, BaseMessage &message)
{
    const qint64 startPos = data->pos();

    if (message.isValid()) { // incomplete message from last parse
        message.content.append(data->read(message.contentLength - message.content.length()));
        return;
    }

    while (!data->atEnd()) {
        const QByteArray &headerFieldLine = data->readLine();
        if (headerFieldLine == headerSeparator) {
            if (message.isValid())
                message.content = data->read(message.contentLength);
            return;
        }
        const QPair<QByteArray, QByteArray> nameAndValue = splitHeaderFieldLine(headerFieldLine);
        const QByteArray &headerFieldName = nameAndValue.first.trimmed();
        const QByteArray &headerFieldValue = nameAndValue.second.trimmed();

        if (headerFieldName.isEmpty())
            continue;
        if (headerFieldName == contentLengthFieldName) {
            parseContentLength(message, headerFieldValue, parseError);
        } else if (headerFieldName == contentTypeFieldName) {
            parseContentType(message, headerFieldValue, parseError);
        } else {
            qCWarning(parseLog) << "Unexpected header field" << QLatin1String(headerFieldName)
                                  << "in" << QLatin1String(headerFieldLine);
        }
    }

    // the complete header wasn't received jet, waiting for the rest of it and reparse
    message = BaseMessage();
    data->seek(startPos);
}

TextEncoding BaseMessage::defaultEncoding()
{
    static const TextEncoding encoding(defaultCharset);
    return encoding;
}

bool BaseMessage::isComplete() const
{
    if (!isValid())
        return false;
    QTC_ASSERT(content.length() <= contentLength, return true);
    return content.length() == contentLength;
}

bool BaseMessage::isValid() const
{
    return contentLength >= 0;
}

QByteArray BaseMessage::header() const
{
    QByteArray header;
    header.append(lengthHeader());
    if (encoding != defaultEncoding()
            || (!mimeType.isEmpty() && mimeType != JsonRpcMessage::jsonRpcMimeType())) {
        header.append(typeHeader());
    }
    header.append(headerSeparator);
    return header;
}

QByteArray BaseMessage::lengthHeader() const
{
    return QByteArray(contentLengthFieldName)
            + QByteArray(headerFieldSeparator)
            + QString::number(content.size()).toLatin1()
            + QByteArray(headerSeparator);
}

QByteArray BaseMessage::typeHeader() const
{
    return QByteArray(contentTypeFieldName)
            + QByteArray(headerFieldSeparator)
            + mimeType + "; " + contentCharsetName + "=" + encoding.name()
            + QByteArray(headerSeparator);
}

} // namespace LanguageServerProtocol
