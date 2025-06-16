// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textfileformat.h"

#include "fileutils.h"
#include "qtcassert.h"
#include "stringutils.h"
#include "utilstr.h"

#include <QDebug>

enum { debug = 0 };

namespace Utils {

QDebug operator<<(QDebug d, const TextFileFormat &format)
{
    QDebug nsp = d.nospace();
    nsp << "TextFileFormat: " << format.codec().fullDisplayName()
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

/*!
    Detects the format of text \a data.
*/

void TextFileFormat::detectFromData(const QByteArray &data)
{
    if (data.isEmpty())
        return;
    const int bytesRead = data.size();
    const auto buf = reinterpret_cast<const unsigned char *>(data.constData());
    // code taken from qtextstream
    if (bytesRead >= 4 && ((buf[0] == 0xff && buf[1] == 0xfe && buf[2] == 0 && buf[3] == 0)
                           || (buf[0] == 0 && buf[1] == 0 && buf[2] == 0xfe && buf[3] == 0xff))) {
        m_codec = TextCodec::utf32();
    } else if (bytesRead >= 2 && ((buf[0] == 0xff && buf[1] == 0xfe)
                                  || (buf[0] == 0xfe && buf[1] == 0xff))) {
        m_codec = TextCodec::utf16();
    } else if (bytesRead >= 3 && ((buf[0] == 0xef && buf[1] == 0xbb) && buf[2] == 0xbf)) {
        m_codec = TextCodec::utf8();
        hasUtf8Bom = true;
    }
    // end code taken from qtextstream
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

TextCodec TextFileFormat::codec() const
{
    return m_codec;
}

void TextFileFormat::setCodec(const TextCodec &codec)
{
    m_codec = codec;
}

TextEncoding TextFileFormat::encoding() const
{
    return m_codec.name();
}

void TextFileFormat::setEncoding(const TextEncoding &encoding)
{
    m_codec = TextCodec::codecForName(encoding.name());
}

enum { textChunkSize = 65536 };

static bool verifyDecodingError(const QString &text, const TextCodec codec,
                                const char *data, const int dataSize,
                                const bool possibleHeader)
{
    const QByteArray verifyBuf = codec.fromUnicode(text); // slow
    // the minSize trick lets us ignore unicode headers
    const int minSize = qMin(verifyBuf.size(), dataSize);
    return (minSize < dataSize - (possibleHeader? 4 : 0)
            || memcmp(verifyBuf.constData() + verifyBuf.size() - minSize,
                      data + dataSize - minSize,
                      minSize));
}

/*!
    Decode a potentially large file in chunks and append it to \a target.

    Returns \a data decoded to a string, \a target.
*/

bool TextFileFormat::decode(const QByteArray &dataBA, QString *target) const
{
    QTC_ASSERT(m_codec.isValid(), return false);

    TextCodec::ConverterState state;
    bool hasDecodingError = false;

    target->clear();
    target->reserve(dataBA.size()); // Upper bound.

    const char *start = dataBA.constData();
    const char *data = start;
    const char *end  = data + dataBA.size();
    // Process chunkwise as QTextCodec allocates too much memory when doing it in one
    // go. An alternative to the code below would be creating a decoder from the codec,
    // but its failure detection does not seem be working reliably.
    for (const char *data = start; data < end; ) {
        const char *chunkStart = data;
        const int chunkSize = qMin(int(textChunkSize), int(end - chunkStart));
        QString text = m_codec.toUnicode(chunkStart, chunkSize, &state);
        data += chunkSize;
        // Process until the end of the current multi-byte character. Remaining might
        // actually contain more than needed so try one-be-one. If EOF is reached with
        // and characters remain->encoding error.
        for ( ; state.remainingChars && data < end ; ++data)
            text.append(m_codec.toUnicode(data, 1, &state));
        if (state.remainingChars)
            hasDecodingError = true;
        if (!hasDecodingError)
            hasDecodingError =
                verifyDecodingError(text, m_codec, chunkStart, data - chunkStart,
                                    chunkStart == start);
        if (lineTerminationMode == TextFileFormat::CRLFLineTerminator)
            text.remove(QLatin1Char('\r'));
        target->append(text);
    }
    return !hasDecodingError;
}

/*!
    Reads a text file from \a filePath into a string, \a plainText using
    \a defaultCodec and text file format \a format.

    Returns whether decoding was possible without errors. If an errors occur
    it is returned together with a decoding error sample.

    \note This function does \e{not} use the codec set by \l setCodec. Instead
    it detects the codec to be used from BOM read file contents.
    If none is present, it falls back using the provided \a fallbackCodec.
    If this still doesn't exist, it uses \l TextEncoding::encodingForLocale()
*/

TextFileFormat::ReadResult
TextFileFormat::readFile(const FilePath &filePath, const TextEncoding &fallbackCodec)
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

    if (!m_codec.isValid())
        m_codec = TextCodec::codecForName(fallbackCodec.name());
    if (!m_codec.isValid())
        m_codec = TextCodec::codecForLocale();

    TextFileFormat::ReadResult result;
    if (!decode(data, &result.content)) {
        result.decodingErrorSample = TextFileFormat::decodingErrorSample(data);
        return {TextFileFormat::ReadEncodingError, Tr::tr("An encoding error was encountered.")};
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
    if (!format.m_codec.isValid())
        format.m_codec = TextCodec::codecForName(fallbackEncoding.name());
    if (!format.m_codec.isValid())
        format.m_codec = TextCodec::codecForLocale();

    QString target;
    if (format.m_codec.isUtf8() || !format.decode(data, &target)) {
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

    Returns whether decoding was possible without errors. If errors occur,
    returns an error message, \a errorString.
*/

Result<> TextFileFormat::writeFile(const FilePath &filePath, QString plainText) const
{
    QTC_ASSERT(m_codec.isValid(), return ResultError("No codec"));

    // Does the user want CRLF? If that is native,
    // do not let QFile do the work, because it replaces the line ending after the text was encoded,
    // and this could lead to undecodable file contents.
    QIODevice::OpenMode fileMode = QIODevice::NotOpen;
    if (lineTerminationMode == CRLFLineTerminator)
        plainText.replace(QLatin1Char('\n'), QLatin1String("\r\n"));

    FileSaver saver(filePath, fileMode);
    if (!saver.hasError()) {
        if (hasUtf8Bom && m_codec.isUtf8())
            saver.write({"\xef\xbb\xbf", 3});
        saver.write(m_codec.fromUnicode(plainText));
    }

    const Result<> result = saver.finalize();
    if (debug)
        qDebug().nospace() << Q_FUNC_INFO << filePath << ' ' << *this <<  ' ' << plainText.size()
                           << " bytes, returns " << result.has_value();
    return result;
}

} // namespace Utils
