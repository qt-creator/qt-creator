// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textfileformat.h"

#include "fileutils.h"
#include "qtcassert.h"
#include "utilstr.h"

#include <QDebug>
#include <QTextCodec>

enum { debug = 0 };

namespace Utils {

QDebug operator<<(QDebug d, const TextFileFormat &format)
{
    QDebug nsp = d.nospace();
    nsp << "TextFileFormat: ";
    if (format.codec) {
        nsp << format.codec->name();
        const QList<QByteArray> aliases = format.codec->aliases();
        for (const QByteArray &alias : aliases)
            nsp << ' ' << alias;
    } else {
        nsp << "NULL";
    }
    nsp << " hasUtf8Bom=" << format.hasUtf8Bom
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
    \li Encoding represented by a pointer to a QTextCodec
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

TextFileFormat TextFileFormat::detect(const QByteArray &data)
{
    TextFileFormat result;
    if (data.isEmpty())
        return result;
    const int bytesRead = data.size();
    const auto buf = reinterpret_cast<const unsigned char *>(data.constData());
    // code taken from qtextstream
    if (bytesRead >= 4 && ((buf[0] == 0xff && buf[1] == 0xfe && buf[2] == 0 && buf[3] == 0)
                           || (buf[0] == 0 && buf[1] == 0 && buf[2] == 0xfe && buf[3] == 0xff))) {
        result.codec = QTextCodec::codecForName("UTF-32");
    } else if (bytesRead >= 2 && ((buf[0] == 0xff && buf[1] == 0xfe)
                                  || (buf[0] == 0xfe && buf[1] == 0xff))) {
        result.codec = QTextCodec::codecForName("UTF-16");
    } else if (bytesRead >= 3 && ((buf[0] == 0xef && buf[1] == 0xbb) && buf[2] == 0xbf)) {
        result.codec = QTextCodec::codecForName("UTF-8");
        result.hasUtf8Bom = true;
    }
    // end code taken from qtextstream
    const int newLinePos = data.indexOf('\n');
    if (newLinePos == -1)
        result.lineTerminationMode = NativeLineTerminator;
    else if (newLinePos == 0)
        result.lineTerminationMode = LFLineTerminator;
    else
        result.lineTerminationMode = data.at(newLinePos - 1) == '\r' ? CRLFLineTerminator : LFLineTerminator;
    return result;
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

enum { textChunkSize = 65536 };

static bool verifyDecodingError(const QString &text, const QTextCodec *codec,
                                const char *data, const int dataSize,
                                const bool possibleHeader)
{
    const QByteArray verifyBuf = codec->fromUnicode(text); // slow
    // the minSize trick lets us ignore unicode headers
    const int minSize = qMin(verifyBuf.size(), dataSize);
    return (minSize < dataSize - (possibleHeader? 4 : 0)
            || memcmp(verifyBuf.constData() + verifyBuf.size() - minSize,
                      data + dataSize - minSize,
                      minSize));
}

// Decode a potentially large file in chunks and append it to target
// using the append function passed on (fits QStringList and QString).

template <class Target>
bool decodeTextFileContent(const QByteArray &dataBA,
                           const TextFileFormat &format,
                           Target *target,
                           void (Target::*appendFunction)(const QString &))
{
    QTC_ASSERT(format.codec, return false);

    QTextCodec::ConverterState state;
    bool hasDecodingError = false;

    const char *start = dataBA.constData();
    const char *data = start;
    const char *end  = data + dataBA.size();
    // Process chunkwise as QTextCodec allocates too much memory when doing it in one
    // go. An alternative to the code below would be creating a decoder from the codec,
    // but its failure detection does not seem be working reliably.
    for (const char *data = start; data < end; ) {
        const char *chunkStart = data;
        const int chunkSize = qMin(int(textChunkSize), int(end - chunkStart));
        QString text = format.codec->toUnicode(chunkStart, chunkSize, &state);
        data += chunkSize;
        // Process until the end of the current multi-byte character. Remaining might
        // actually contain more than needed so try one-be-one. If EOF is reached with
        // and characters remain->encoding error.
        for ( ; state.remainingChars && data < end ; ++data)
            text.append(format.codec->toUnicode(data, 1, &state));
        if (state.remainingChars)
            hasDecodingError = true;
        if (!hasDecodingError)
            hasDecodingError =
                verifyDecodingError(text, format.codec, chunkStart, data - chunkStart,
                                    chunkStart == start);
        if (format.lineTerminationMode == TextFileFormat::CRLFLineTerminator)
            text.remove(QLatin1Char('\r'));
        (target->*appendFunction)(text);
    }
    return !hasDecodingError;
}

/*!
    Returns \a data decoded to a plain string, \a target.
*/

bool TextFileFormat::decode(const QByteArray &data, QString *target) const
{
    target->clear();
    return decodeTextFileContent<QString>(data, *this, target, &QString::push_back);
}

/*!
    Returns \a data decoded to a list of strings, \a target.

    Intended for use with progress bars loading large files.
*/

bool TextFileFormat::decode(const QByteArray &data, QStringList *target) const
{
    target->clear();
    if (data.size() > textChunkSize)
        target->reserve(5 + data.size() / textChunkSize);
    return decodeTextFileContent<QStringList>(data, *this, target, &QStringList::append);
}

// Read text file contents to string or stringlist.
template <class Target>
TextFileFormat::ReadResult readTextFile(const FilePath &filePath, const QTextCodec *defaultCodec,
                                        Target *target, TextFileFormat *format, QString *errorString,
                                        QByteArray *decodingErrorSampleIn = nullptr)
{
    if (decodingErrorSampleIn)
        decodingErrorSampleIn->clear();

    QByteArray data;
    try {
        FileReader reader;
        if (!reader.fetch(filePath, errorString))
            return TextFileFormat::ReadIOError;
        data = reader.data();
    } catch (const std::bad_alloc &) {
        *errorString = Tr::tr("Out of memory.");
        return TextFileFormat::ReadMemoryAllocationError;
    }

    if (!data.isEmpty())
        *format = TextFileFormat::detect(data);

    if (!format->codec)
        format->codec = defaultCodec ? defaultCodec : QTextCodec::codecForLocale();

    if (!format->decode(data, target)) {
        *errorString = Tr::tr("An encoding error was encountered.");
        if (decodingErrorSampleIn)
            *decodingErrorSampleIn = TextFileFormat::decodingErrorSample(data);
        return TextFileFormat::ReadEncodingError;
    }
    return TextFileFormat::ReadSuccess;
}

/*!
    Reads a text file from \a filePath into a list of strings, \a plainTextList
    using \a defaultCodec and text file format \a format.

    Returns whether decoding was possible without errors. If errors occur,
    returns an error message, \a errorString and a sample error,
    \a decodingErrorSample.
*/

TextFileFormat::ReadResult
    TextFileFormat::readFile(const FilePath &filePath, const QTextCodec *defaultCodec,
                             QStringList *plainTextList, TextFileFormat *format, QString *errorString,
                             QByteArray *decodingErrorSample /* = 0 */)
{
    const TextFileFormat::ReadResult result =
        readTextFile(filePath, defaultCodec,
                     plainTextList, format, errorString, decodingErrorSample);
    if (debug)
        qDebug().nospace() << Q_FUNC_INFO << filePath << ' ' << *format
                           << " returns " << result << '/' << plainTextList->size() << " chunks";
    return result;
}

/*!
    Reads a text file from \a filePath into a string, \a plainText using
    \a defaultCodec and text file format \a format.

    Returns whether decoding was possible without errors.

*/

TextFileFormat::ReadResult
    TextFileFormat::readFile(const FilePath &filePath, const QTextCodec *defaultCodec,
                             QString *plainText, TextFileFormat *format, QString *errorString,
                             QByteArray *decodingErrorSample /* = 0 */)
{
    const TextFileFormat::ReadResult result =
        readTextFile(filePath, defaultCodec,
                     plainText, format, errorString, decodingErrorSample);
    if (debug)
        qDebug().nospace() << Q_FUNC_INFO << filePath << ' ' << *format
                           << " returns " << result << '/' << plainText->size() << " characters";
    return result;
}

TextFileFormat::ReadResult TextFileFormat::readFileUTF8(const FilePath &filePath,
                                                        const QTextCodec *defaultCodec,
                                                        QByteArray *plainText, QString *errorString)
{
    QByteArray data;
    try {
        FileReader reader;
        if (!reader.fetch(filePath, errorString))
            return TextFileFormat::ReadIOError;
        data = reader.data();
    } catch (const std::bad_alloc &) {
        *errorString = Tr::tr("Out of memory.");
        return TextFileFormat::ReadMemoryAllocationError;
    }

    TextFileFormat format = TextFileFormat::detect(data);
    if (!format.codec)
        format.codec = defaultCodec ? defaultCodec : QTextCodec::codecForLocale();
    QString target;
    if (format.codec->name() == "UTF-8" || !format.decode(data, &target)) {
        if (format.hasUtf8Bom)
            data.remove(0, 3);
        if (format.lineTerminationMode == TextFileFormat::CRLFLineTerminator)
            data.replace("\r\n", "\n");
        *plainText = data;
        return TextFileFormat::ReadSuccess;
    }
    *plainText = target.toUtf8();
    return TextFileFormat::ReadSuccess;
}

tl::expected<QString, std::pair<TextFileFormat::ReadResult, QString>>
TextFileFormat::readFile(const FilePath &filePath, const QTextCodec *defaultCodec)
{
    QString plainText;
    TextFileFormat format;
    QString errorString;
    const TextFileFormat::ReadResult result =
        readTextFile(filePath, defaultCodec, &plainText, &format, &errorString, nullptr);
    if (result != TextFileFormat::ReadSuccess)
        return tl::unexpected(std::make_pair(result, errorString));
    return plainText;
}

/*!
    Writes out a text file to \a filePath into a string, \a plainText.

    Returns whether decoding was possible without errors. If errors occur,
    returns an error message, \a errorString.
*/

bool TextFileFormat::writeFile(const FilePath &filePath, QString plainText, QString *errorString) const
{
    QTC_ASSERT(codec, return false);

    // Does the user want CRLF? If that is native,
    // do not let QFile do the work, because it replaces the line ending after the text was encoded,
    // and this could lead to undecodable file contents.
    QIODevice::OpenMode fileMode = QIODevice::NotOpen;
    if (lineTerminationMode == CRLFLineTerminator)
        plainText.replace(QLatin1Char('\n'), QLatin1String("\r\n"));

    FileSaver saver(filePath, fileMode);
    if (!saver.hasError()) {
        if (hasUtf8Bom && codec->name() == "UTF-8")
            saver.write("\xef\xbb\xbf", 3);
        saver.write(codec->fromUnicode(plainText));
    }
    const bool ok = saver.finalize(errorString);
    if (debug)
        qDebug().nospace() << Q_FUNC_INFO << filePath << ' ' << *this <<  ' ' << plainText.size()
                           << " bytes, returns " << ok;
    return ok;
}

} // namespace Utils
