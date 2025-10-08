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
        m_encoding = TextEncoding::Utf32;
    } else if (bytesRead >= 2 && ((buf[0] == 0xff && buf[1] == 0xfe)
                                  || (buf[0] == 0xfe && buf[1] == 0xff))) {
        m_encoding = TextEncoding::Utf16;
    } else if (bytesRead >= 3 && ((buf[0] == 0xef && buf[1] == 0xbb) && buf[2] == 0xbf)) {
        m_encoding = TextEncoding::Utf8;
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

    if (decoder.hasError())
        return false;

    if (lineTerminationMode == TextFileFormat::CRLFLineTerminator)
        target->remove(QLatin1Char('\r'));

    return true;
}

/*!
    Reads a text file from \a filePath into a string. It detects the codec to use
    from the BOM read file contents. If none is set, it uses \a fallbackEncoding.
    If the fallback is not set, it uses the text encoding set for the locale.

    Returns whether decoding was possible without errors. If an error occurs,
    it is returned together with a decoding error sample.
*/

TextFileFormat::ReadResult
TextFileFormat::readFile(const FilePath &filePath, const TextEncoding &fallbackEncoding)
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

    if (!m_encoding.isValid())
        m_encoding = fallbackEncoding;
    if (!m_encoding.isValid())
        m_encoding = TextEncoding::encodingForLocale();

    TextFileFormat::ReadResult result;
    if (!decode(data, &result.content)) {
        result.code = TextFileFormat::ReadEncodingError;
        result.error = Tr::tr("An encoding error was encountered.");
        result.decodingErrorSample = TextFileFormat::decodingErrorSample(data);
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
    if (debug)
        qDebug().nospace() << Q_FUNC_INFO << filePath << ' ' << *this <<  ' ' << plainText.size()
                           << " bytes, returns " << result.has_value();
    return result;
}

} // namespace Utils
