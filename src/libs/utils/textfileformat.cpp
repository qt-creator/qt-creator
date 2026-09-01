// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textfileformat.h"

#include "fileutils.h"
#include "qtcassert.h"
#include "stringutils.h"
#include "utilstr.h"

#include <QDebug>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(logTextFileFormat, "qtc.utils.textfileformat", QtWarningMsg)

namespace Utils {

static QDebug operator<<(QDebug d, const TextFileFormat &format)
{
    QDebug nsp = d.nospace();
    nsp << "TextFileFormat: " << format.encoding().fullDisplayName()
        << " hasUtf8Bom=" << format.hasUtf8Bom
        << (format.lineTerminationMode == TextFileFormat::LFLineTerminator ? " LF" : " CRLF");
    return d;
}

/*!
    \class Utils::TextFileFormat
    \inmodule QtCreator

    \brief The TextFileFormat class describes the format of a text file and
    provides autodetection.

    The format comprises
    \list
    \li Encoding represented by a the name of a codec
    \li Presence of an UTF8 Byte Order Marker (BOM)
    \li Line feed storage convention
    \endlist

    The class also provides convenience functions to read text files and return them
    as strings or string lists and to write out files.
*/

TextFileFormat::TextFileFormat() = default;

static TextEncoding encodingFromBom(const QByteArray &data)
{
    if (data.startsWith(QByteArrayView("\xff\xfe\0\0", 4))
        || data.startsWith(QByteArrayView("\0\0\xfe\xff", 4))) {
        return TextEncoding(TextEncoding::Utf32);
    }
    if (data.startsWith("\xff\xfe") || data.startsWith("\xfe\xff"))
        return TextEncoding(TextEncoding::Utf16);
    if (data.startsWith("\xef\xbb\xbf"))
        return TextEncoding(TextEncoding::Utf8);
    return {};
}

/*!
    Detects the format of text \a data.
*/

void TextFileFormat::detectFromData(const QByteArray &data)
{
    if (data.isEmpty())
        return;
    if (const TextEncoding bomEncoding = encodingFromBom(data); bomEncoding.isValid()) {
        m_encoding = bomEncoding;
        if (bomEncoding.isUtf8())
            hasUtf8Bom = true;
    }
    const int newLinePos = data.indexOf('\n');
    if (newLinePos == -1)
        lineTerminationMode = NativeLineTerminator;
    else if (newLinePos == 0)
        lineTerminationMode = LFLineTerminator;
    else
        lineTerminationMode = data.at(newLinePos - 1) == '\r' ? CRLFLineTerminator : LFLineTerminator;
}

/*!
    Returns a piece of text specified by \a data suitable as display for
    an encoding error.
*/

QByteArray TextFileFormat::decodingErrorSample(const QByteArray &data)
{
    const int p = data.indexOf('\n', 16384);
    return p < 0 ? data : data.left(p);
}

static int utf8SequenceLength(const QByteArray &data, qsizetype offset)
{
    const auto byte = static_cast<unsigned char>(data.at(offset));
    if (byte <= 0x7f)
        return 1;

    int length = 0;
    unsigned char secondByteMin = 0x80;
    unsigned char secondByteMax = 0xbf;
    if (byte >= 0xc2 && byte <= 0xdf) {
        length = 2;
    } else if (byte == 0xe0) {
        length = 3;
        secondByteMin = 0xa0;
    } else if ((byte >= 0xe1 && byte <= 0xec) || (byte >= 0xee && byte <= 0xef)) {
        length = 3;
    } else if (byte == 0xed) {
        length = 3;
        secondByteMax = 0x9f;
    } else if (byte == 0xf0) {
        length = 4;
        secondByteMin = 0x90;
    } else if (byte >= 0xf1 && byte <= 0xf3) {
        length = 4;
    } else if (byte == 0xf4) {
        length = 4;
        secondByteMax = 0x8f;
    }

    bool valid = length > 0 && offset + length <= data.size();
    for (int i = 1; valid && i < length; ++i) {
        const auto continuation = static_cast<unsigned char>(data.at(offset + i));
        const unsigned char minimum = i == 1 ? secondByteMin : 0x80;
        const unsigned char maximum = i == 1 ? secondByteMax : 0xbf;
        valid = continuation >= minimum && continuation <= maximum;
    }
    return valid ? length : 0;
}

/*!
    Maps ascending byte offsets to UTF-16 positions in the decoded text.
*/
QList<qsizetype> TextFileFormat::decodedPositionsForByteOffsets(
    const QByteArray &data, const TextEncoding &encoding,
    const QList<qsizetype> &byteOffsets, LineTerminationMode lineTerminationMode)
{
    QStringDecoder decoder(encoding.name());
    QList<qsizetype> positions;
    positions.reserve(byteOffsets.size());
    qsizetype decodedPosition = 0;
    qsizetype decodedByteOffset = encoding.isUtf8() && data.startsWith("\xef\xbb\xbf") ? 3 : 0;
    bool previousCarriageReturn = false;
    const auto countCharacters = [&](QStringView decoded) {
        for (QChar character : decoded) {
            if (character == QLatin1Char('\n') && previousCarriageReturn
                && lineTerminationMode != CRLFLineTerminator) {
                --decodedPosition;
            }
            if (character != QLatin1Char('\r') || lineTerminationMode != CRLFLineTerminator)
                ++decodedPosition;
            previousCarriageReturn = character == QLatin1Char('\r');
        }
    };

    for (qsizetype offset : byteOffsets) {
        const qsizetype targetOffset = qBound(decodedByteOffset, offset, qsizetype(data.size()));
        if (encoding.isUtf8()) {
            while (decodedByteOffset < targetOffset) {
                const int length = qMax(1, utf8SequenceLength(data, decodedByteOffset));
                if (decodedByteOffset + length > targetOffset)
                    break;
                const auto byte = static_cast<unsigned char>(data.at(decodedByteOffset));
                if (byte < 0x80) {
                    const QChar character{char16_t(byte)};
                    countCharacters(QStringView(&character, 1));
                } else {
                    decodedPosition += length == 4 ? 2 : 1;
                    previousCarriageReturn = false;
                }
                decodedByteOffset += length;
            }
        } else {
            const QString decoded = decoder.decode(QByteArrayView(data).sliced(
                decodedByteOffset, targetOffset - decodedByteOffset));
            countCharacters(decoded);
            decodedByteOffset = targetOffset;
        }
        positions.append(decodedPosition);
    }
    return positions;
}

static QList<qsizetype> findDecodingErrorOffsets(const QByteArray &data,
                                                 const TextEncoding &encoding)
{
    constexpr qsizetype maximumOffsets = TextFileFormat::MaximumDecodingErrorOffsets + 1;

    if (encoding.isUtf8()) {
        QList<qsizetype> offsets;
        for (qsizetype offset = 0; offset < data.size() && offsets.size() < maximumOffsets;) {
            const int length = utf8SequenceLength(data, offset);
            if (length) {
                offset += length;
            } else {
                offsets.append(offset);
                ++offset;
            }
        }
        return offsets;
    }

    QStringDecoder decoder(encoding.name());
    QList<qsizetype> offsets;
    qsizetype pendingSequenceOffset = -1;
    qsizetype decoderResetOffset = 0;
    const auto decodeByte = [&](qsizetype offset) {
        const QString decoded = decoder.decode(QByteArrayView(data).sliced(offset, 1));
        if (decoder.hasError()) {
            const qsizetype errorOffset = pendingSequenceOffset >= 0
                                              ? pendingSequenceOffset : offset;
            offsets.append(errorOffset);
            decoder = QStringDecoder(encoding.name());
            decoderResetOffset = offset + 1;
            pendingSequenceOffset = -1;

            // Some decoders only report an incomplete sequence when they see the next byte.
            // Retry that byte from a clean state so it is not lost from the decoded stream.
            if (errorOffset < offset) {
                QStringDecoder retryDecoder(encoding.name());
                const QString retried
                    = retryDecoder.decode(QByteArrayView(data).sliced(offset, 1));
                decoderResetOffset = offset;
                if (retryDecoder.hasError()) {
                    offsets.append(offset);
                    decoder = QStringDecoder(encoding.name());
                    decoderResetOffset = offset + 1;
                } else {
                    decoder = std::move(retryDecoder);
                    if (retried.isEmpty())
                        pendingSequenceOffset = offset;
                }
            }
        } else if (decoded.isEmpty() && pendingSequenceOffset < 0) {
            pendingSequenceOffset = offset;
        } else if (!decoded.isEmpty()) {
            pendingSequenceOffset = -1;
        }
    };

    QStringDecoder chunkDecoder(encoding.name());
    constexpr qsizetype chunkSize = 4096;
    constexpr qsizetype sequenceTailSize = 64;
    for (qsizetype start = 0; start < data.size() && offsets.size() < maximumOffsets;
         start += chunkSize) {
        const qsizetype end = qMin(start + chunkSize, data.size());
        const QByteArrayView chunk = QByteArrayView(data).sliced(start, end - start);
        const QString decoded = chunkDecoder.decode(chunk);
        qsizetype byteStart = start;
        if (!chunkDecoder.hasError()) {
            // Keep the sequence start for a character spanning the next chunk boundary.
            const qsizetype bulkSize = qMax(qsizetype(0), chunk.size() - sequenceTailSize);
            const QString bulk = decoder.decode(chunk.first(bulkSize));
            if (!bulk.isEmpty())
                pendingSequenceOffset = -1;
            byteStart += bulkSize;
        }
        for (qsizetype offset = byteStart; offset < end && offsets.size() < maximumOffsets;
             ++offset) {
            decodeByte(offset);
        }
        if (chunkDecoder.hasError()) {
            chunkDecoder = QStringDecoder(encoding.name());
            const QString restored = chunkDecoder.decode(QByteArrayView(data).sliced(
                decoderResetOffset, end - decoderResetOffset));
        }
    }
    return offsets.sliced(0, qMin(offsets.size(), maximumOffsets));
}

TextEncoding TextFileFormat::encoding() const
{
    return m_encoding;
}

void TextFileFormat::setEncoding(const TextEncoding &encoding)
{
    m_encoding = encoding;
}

/*!
    Returns \a data decoded to a string, \a target.
*/

bool TextFileFormat::decode(const QByteArray &data, QString *target) const
{
    QTC_ASSERT(m_encoding.isValid(), return false);

    QStringDecoder decoder(m_encoding.name());
    *target = decoder.decode(data);

    if (lineTerminationMode == TextFileFormat::CRLFLineTerminator)
        target->remove(QLatin1Char('\r'));

    return !decoder.hasError();
}

/*!
    Reads a text file from \a filePath into a string. It detects the codec to use
    from the BOM in the file contents. If there is no BOM and no encoding is set,
    it uses \a defaultEncoding. If that is not set, it uses the text encoding
    set for the locale.

    If decoding with the primary encoding fails, the method retries with
    \a retryEncoding unless a BOM is present or \a retryEncoding equals the
    primary encoding. On a successful retry, \a retryEncoding becomes the
    format's encoding and \c ReadResult::usedDecodingFallback is set.

    With \a errorDetails set to Collect, retains the failed input and collects
    the first 1001 error offsets for editor diagnostics. Other callers skip
    the diagnostic scan.

    Returns whether decoding was possible without errors. If an error occurs,
    it is returned together with a decoding error sample.
*/

TextFileFormat::ReadResult
TextFileFormat::readFile(const FilePath &filePath,
                        const TextEncoding &defaultEncoding,
                        const TextEncoding &retryEncoding,
                        DecodingErrorDetails errorDetails)
{
    QByteArray data;
    try {
        const Result<QByteArray> res = filePath.fileContents();
        if (!res)
            return {TextFileFormat::ReadIOError, res.error()};
        data = *res;
    } catch (const std::bad_alloc &) {
        return {TextFileFormat::ReadMemoryAllocationError, Tr::tr("Out of memory.")};
    }

    detectFromData(data);
    const bool hasBom = encodingFromBom(data).isValid();

    if (!m_encoding.isValid())
        m_encoding = defaultEncoding;
    if (!m_encoding.isValid())
        m_encoding = TextEncoding::encodingForLocale();

    TextFileFormat::ReadResult result;
    if (!decode(data, &result.content)) {
        if (!hasBom && retryEncoding.isValid() && retryEncoding != m_encoding) {
            const TextEncoding failedEncoding = m_encoding;
            const QString failedContent = result.content;
            m_encoding = retryEncoding;
            if (decode(data, &result.content)) {
                result.usedDecodingFallback = true;
                return result;
            }
            m_encoding = failedEncoding;
            result.content = failedContent;
        }

        result.code = TextFileFormat::ReadEncodingError;
        result.error = Tr::tr("An encoding error was encountered.");
        result.decodingErrorSample = TextFileFormat::decodingErrorSample(data);
        if (errorDetails == DecodingErrorDetails::Skip)
            return result;
        result.decodingErrorOffsets = findDecodingErrorOffsets(data, m_encoding);
        result.decodingErrorData = data;
        if (!result.decodingErrorOffsets.isEmpty()) {
            result.decodingErrorOffset = result.decodingErrorOffsets.first();
            result.decodingErrorPosition
                = decodedPositionsForByteOffsets(data, m_encoding,
                                                 {result.decodingErrorOffset},
                                                 lineTerminationMode).value(0);
        }

        return result;
    }
    return result;
}

Result<> TextFileFormat::readFileUtf8(const FilePath &filePath,
                                      const TextEncoding &fallbackEncoding,
                                      QByteArray *plainText)
{
    QByteArray data;
    try {
        const Result<QByteArray> res = filePath.fileContents();
        if (!res)
            return ResultError(res.error());
        data = *res;
    } catch (const std::bad_alloc &) {
        return ResultError(Tr::tr("Out of memory."));
    }

    TextFileFormat format;
    format.detectFromData(data);
    if (!format.m_encoding.isValid())
        format.m_encoding = fallbackEncoding;
    if (!format.m_encoding.isValid())
        format.m_encoding = TextEncoding::encodingForLocale();

    QString target;
    if (format.m_encoding.isUtf8() || !format.decode(data, &target)) {
        if (format.hasUtf8Bom)
            data.remove(0, 3);
        if (format.lineTerminationMode == TextFileFormat::CRLFLineTerminator)
            data.replace("\r\n", "\n");
        *plainText = data;
    } else {
        *plainText = target.toUtf8();
    }
    return ResultOk;
}

/*!
    Writes out a text file to \a filePath into a string, \a plainText.

    Returns whether decoding was possible without errors.
*/

Result<> TextFileFormat::writeFile(const FilePath &filePath, QString plainText) const
{
    QTC_ASSERT(m_encoding.isValid(), return ResultError("No codec"));

    // Does the user want CRLF? If that is native,
    // do not let QFile do the work, because it replaces the line ending after the text was encoded,
    // and this could lead to undecodable file contents.
    QIODevice::OpenMode fileMode = QIODevice::NotOpen;
    if (lineTerminationMode == CRLFLineTerminator)
        plainText.replace(QLatin1Char('\n'), QLatin1String("\r\n"));

    FileSaver saver(filePath, fileMode);
    if (!saver.hasError()) {
        if (hasUtf8Bom && m_encoding.isUtf8())
            saver.write({"\xef\xbb\xbf", 3});
        saver.write(m_encoding.encode(plainText));
    }

    const Result<> result = saver.finalize();
    qCDebug(logTextFileFormat).nospace()
        << Q_FUNC_INFO << filePath << ' ' << *this << ' ' << plainText.size() << " bytes, returns "
        << result.has_value();
    return result;
}

} // namespace Utils
