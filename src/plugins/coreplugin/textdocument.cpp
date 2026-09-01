// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textdocument.h"

#include "editormanager/editormanager.h"

#include <QDebug>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(logBaseTextDocument, "qtc.core.basetextdocument", QtWarningMsg)

/*!
    \class Core::BaseTextDocument
    \inheaderfile coreplugin/textdocument.h
    \inmodule QtCreator

    \brief The BaseTextDocument class is a very general base class for
    documents that work with text.

    This class contains helper methods for saving and reading text files with encoding and
    line ending settings.

    \sa Utils::TextFileFormat
*/

using namespace Utils;

namespace Core {

namespace Internal {

class TextDocumentPrivate
{
public:
    TextFileFormat m_format;
    QList<TextEncoding> m_supportedEncodings; // empty means all are supported
    bool m_supportsUtf8Bom = true;
    bool m_hasDecodingError = false;
    TextEncoding m_failedPrimaryEncoding;
    QByteArray m_decodingErrorSample;
    QByteArray m_decodingErrorData;
    qsizetype m_decodingErrorOffset = -1;
    qsizetype m_decodingErrorPosition = -1;
    QList<qsizetype> m_decodingErrorOffsets;
    bool m_decodingFallbackEnabled = false;
    bool m_collectDecodingErrorDetails = false;
};

} // namespace Internal

BaseTextDocument::BaseTextDocument(QObject *parent)
    : IDocument(parent), d(new Internal::TextDocumentPrivate)
{
    setEncoding(Core::EditorManager::defaultTextEncoding());
    setLineTerminationMode(Core::EditorManager::defaultLineEnding());
}

BaseTextDocument::~BaseTextDocument()
{
    delete d;
}

bool BaseTextDocument::hasDecodingError() const
{
    return d->m_hasDecodingError;
}

bool BaseTextDocument::decodedWithFallback() const
{
    return d->m_failedPrimaryEncoding.isValid();
}

TextEncoding BaseTextDocument::failedPrimaryEncoding() const
{
    return d->m_failedPrimaryEncoding;
}

QByteArray BaseTextDocument::decodingErrorSample() const
{
    return d->m_decodingErrorSample;
}

QByteArray BaseTextDocument::decodingErrorData() const
{
    return d->m_decodingErrorData;
}

qsizetype BaseTextDocument::decodingErrorOffset() const
{
    return d->m_decodingErrorOffset;
}

qsizetype BaseTextDocument::decodingErrorPosition() const
{
    return d->m_decodingErrorPosition;
}

QList<qsizetype> BaseTextDocument::decodingErrorOffsets() const
{
    return d->m_decodingErrorOffsets;
}

bool BaseTextDocument::isDecodingFallbackEnabled() const
{
    return d->m_decodingFallbackEnabled;
}

void BaseTextDocument::setDecodingFallbackEnabled(bool enabled)
{
    d->m_decodingFallbackEnabled = enabled;
}

/*!
    Writes out the contents (\a data) of the text file \a filePath.
    Uses the format obtained from the last read() of the file.

    Returns whether the operation was successful.
*/

Result<> BaseTextDocument::write(const FilePath &filePath, const QString &data) const
{
    return write(filePath, format(), data);
}

/*!
    Writes out the contents (\a data) of the text file \a filePath.
    Uses the custom format \a format.

    Returns whether the operation was successful.
*/

Result<> BaseTextDocument::write(const FilePath &filePath,
                               const TextFileFormat &format,
                               const QString &data) const
{
    qCDebug(logBaseTextDocument) << Q_FUNC_INFO << this << filePath;
    return format.writeFile(filePath, data);
}

void BaseTextDocument::setSupportsUtf8Bom(bool value)
{
    d->m_supportsUtf8Bom = value;
}

void BaseTextDocument::setLineTerminationMode(TextFileFormat::LineTerminationMode mode)
{
    d->m_format.lineTerminationMode = mode;
}

/*!
    Autodetects file format and reads the text file specified by \a filePath.
    If decoding fails, reads the file again with the configured fallback encoding
    when fallback decoding is enabled and \a fallback is Allowed. When a document
    was previously decoded with the fallback encoding, a later read first retries
    the failed primary encoding.

    Returns whether the operation was successful.
*/

BaseTextDocument::ReadResult BaseTextDocument::read(const FilePath &filePath,
                                                    DecodingFallback fallback)
{
    const bool fallbackEnabled = d->m_decodingFallbackEnabled
                                 && fallback == DecodingFallback::Allowed;
    const TextEncoding previousFailedPrimaryEncoding = d->m_failedPrimaryEncoding;
    const bool retryFailedPrimary = d->m_failedPrimaryEncoding.isValid() && fallbackEnabled;
    const TextEncoding primaryEncoding = retryFailedPrimary
                                             ? d->m_failedPrimaryEncoding : encoding();
    if (retryFailedPrimary)
        d->m_format.setEncoding(primaryEncoding);
    const TextEncoding retryEncoding = fallbackEnabled
                                           ? Core::EditorManager::fallbackTextEncoding()
                                           : TextEncoding();
    ReadResult res = d->m_format.readFile(
        filePath, primaryEncoding,
        supportsEncoding(retryEncoding) ? retryEncoding : TextEncoding(),
        d->m_collectDecodingErrorDetails ? TextFileFormat::DecodingErrorDetails::Collect
                                         : TextFileFormat::DecodingErrorDetails::Skip);
    d->m_hasDecodingError = res.code == TextFileFormat::ReadEncodingError;
    d->m_failedPrimaryEncoding = res.usedDecodingFallback ? primaryEncoding : TextEncoding();
    d->m_decodingErrorSample = res.decodingErrorSample;
    d->m_decodingErrorData = res.decodingErrorData;
    d->m_decodingErrorOffset = res.decodingErrorOffset;
    d->m_decodingErrorPosition = res.decodingErrorPosition;
    d->m_decodingErrorOffsets = res.decodingErrorOffsets;
    if (previousFailedPrimaryEncoding != d->m_failedPrimaryEncoding)
        emit decodingFallbackChanged();
    return res;
}

QString BaseTextDocument::plainText() const
{
    return d->m_format.encoding().decode(contents());
}

void BaseTextDocument::setEncoding(const TextEncoding &encoding)
{
    qCDebug(logBaseTextDocument) << Q_FUNC_INFO << this << encoding.name();
    if (supportsEncoding(encoding)) {
        const bool hadFallback = d->m_failedPrimaryEncoding.isValid();
        d->m_format.setEncoding(encoding);
        d->m_failedPrimaryEncoding = {};
        if (hadFallback)
            emit decodingFallbackChanged();
    }
}

void BaseTextDocument::restoreDecodingFallbackState(const TextEncoding &failedPrimaryEncoding)
{
    const bool changed = d->m_failedPrimaryEncoding != failedPrimaryEncoding;
    d->m_failedPrimaryEncoding = failedPrimaryEncoding;
    if (changed)
        emit decodingFallbackChanged();
}

void BaseTextDocument::setCollectDecodingErrorDetails(bool collect)
{
    d->m_collectDecodingErrorDetails = collect;
}

TextEncoding BaseTextDocument::encoding() const
{
    return d->m_format.encoding();
}

bool BaseTextDocument::supportsEncoding(const TextEncoding &encoding) const
{
    return d->m_supportedEncodings.isEmpty() || d->m_supportedEncodings.contains(encoding);
}

/*!
    Sets the list of supported \a encodings.

    An empty list means that all encodings are supported.
    If the current encoding is not supported, the encoding is set to the first
    supported encoding.
*/
void BaseTextDocument::setSupportedEncodings(const QList<TextEncoding> &encodings)
{
    d->m_supportedEncodings = encodings;
    if (!supportsEncoding(encoding()) && QTC_GUARD(!encodings.isEmpty()))
        d->m_format.setEncoding(encodings.first());
}

void BaseTextDocument::switchUtf8Bom()
{
    qCDebug(logBaseTextDocument) << Q_FUNC_INFO << this << "UTF-8 BOM: " << !d->m_format.hasUtf8Bom;
    d->m_format.hasUtf8Bom = !d->m_format.hasUtf8Bom;
}

bool BaseTextDocument::supportsUtf8Bom() const
{
    return d->m_supportsUtf8Bom;
}

TextFileFormat::LineTerminationMode BaseTextDocument::lineTerminationMode() const
{
    return d->m_format.lineTerminationMode;
}

/*!
    Returns the format obtained from the last call to read().
*/

TextFileFormat BaseTextDocument::format() const
{
    return d->m_format;
}

} // namespace Core
