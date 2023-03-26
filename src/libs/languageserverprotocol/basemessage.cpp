// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "basemessage.h"

#include "jsonrpcmessages.h"
#include "languageserverprotocoltr.h"

#include <QBuffer>
#include <QTextCodec>

#include <cstring>
#include <utility>

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
                         int expectedLength, QTextCodec *codec)
    : mimeType(mimeType.isEmpty() ? JsonRpcMessage::jsonRpcMimeType() : mimeType)
    , content(content)
    , contentLength(expectedLength)
    , codec(codec)
{ }

BaseMessage::BaseMessage(const QByteArray &mimeType, const QByteArray &content)
    : BaseMessage(mimeType, content, content.length(), defaultCodec())
{ }

bool BaseMessage::operator==(const BaseMessage &other) const
{
    if (mimeType != other.mimeType || content != other.content)
        return false;
    if (codec) {
        if (other.codec)
            return codec->mibEnum() == other.codec->mibEnum();
        return codec->mibEnum() == defaultCodec()->mibEnum();
    }
    if (other.codec)
        return other.codec->mibEnum() == defaultCodec()->mibEnum();

    return true;
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
    QTextCodec *codec = nullptr;
    for (const QByteArray &_contentTypeElement : contentTypeElements) {
        const QByteArray &contentTypeElement = _contentTypeElement.trimmed();
        if (contentTypeElement.startsWith(contentCharsetName)) {
            const int equalindex = contentTypeElement.indexOf('=');
            const QByteArray charset = contentTypeElement.mid(equalindex + 1);
            if (equalindex > 0)
                codec = QTextCodec::codecForName(charset);
            if (!codec) {
                parseError = Tr::tr("Cannot decode content with \"%1\". Falling back to \"%2\".")
                                 .arg(QLatin1String(charset),
                                      QLatin1String(defaultCharset));
            }
        }
    }
    message.mimeType = mimeTypeName;
    message.codec = codec ? codec : BaseMessage::defaultCodec();
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

QTextCodec *BaseMessage::defaultCodec()
{
    static QTextCodec *codec = QTextCodec::codecForName(defaultCharset);
    return codec;
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
    if (codec != defaultCodec()
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
            + mimeType + "; " + contentCharsetName + "=" + codec->name()
            + QByteArray(headerSeparator);
}

} // namespace LanguageServerProtocol
