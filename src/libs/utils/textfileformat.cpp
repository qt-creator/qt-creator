/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "textfileformat.h"
#include "fileutils.h"
#include "qtcassert.h"

#include <QTextCodec>
#include <QStringList>
#include <QDebug>

enum { debug = 0 };

namespace Utils {

QDebug operator<<(QDebug d, const TextFileFormat &format)
{
    QDebug nsp = d.nospace();
    nsp << "TextFileFormat: ";
    if (format.codec) {
        nsp << format.codec->name();
        foreach (const QByteArray &alias, format.codec->aliases())
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

    \brief Describes the format of a text file and provides autodetection.

    The format comprises
    \list
    \li Encoding represented by a pointer to a QTextCodec
    \li Presence of an UTF8 Byte Order Marker (BOM)
    \li Line feed storage convention
    \endlist

    The class also provides convenience functions to read text files and return them
    as strings or string lists and to write out files.
*/

TextFileFormat::TextFileFormat() :
    lineTerminationMode(NativeLineTerminator), hasUtf8Bom(false), codec(0)
{
}

/*!
    \brief Detect the format of text data.
*/

TextFileFormat TextFileFormat::detect(const QByteArray &data)
{
    TextFileFormat result;
    if (data.isEmpty())
        return result;
    const int bytesRead = data.size();
    const unsigned char *buf = reinterpret_cast<const unsigned char *>(data.constData());
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
    \brief Returns a piece of text suitable as display for a encoding error.
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
    \brief Decode data to a plain string.
*/

bool TextFileFormat::decode(const QByteArray &data, QString *target) const
{
    target->clear();
    return decodeTextFileContent<QString>(data, *this, target, &QString::push_back);
}

/*!
    \brief Decode data to a list of strings.

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
TextFileFormat::ReadResult readTextFile(const QString &fileName, const QTextCodec *defaultCodec,
                                        Target *target, TextFileFormat *format, QString *errorString,
                                        QByteArray *decodingErrorSampleIn = 0)
{
    if (decodingErrorSampleIn)
        decodingErrorSampleIn->clear();

    QByteArray data;
    try {
        Utils::FileReader reader;
        if (!reader.fetch(fileName, errorString))
            return TextFileFormat::ReadIOError;
        data = reader.data();
    } catch (const std::bad_alloc &) {
        *errorString = QCoreApplication::translate("Utils::TextFileFormat", "Out of memory.");
        return TextFileFormat::ReadMemoryAllocationError;
    }

    *format = TextFileFormat::detect(data);
    if (!format->codec)
        format->codec = defaultCodec ? defaultCodec : QTextCodec::codecForLocale();

    if (!format->decode(data, target)) {
        *errorString = QCoreApplication::translate("Utils::TextFileFormat", "An encoding error was encountered.");
        if (decodingErrorSampleIn)
            *decodingErrorSampleIn = TextFileFormat::decodingErrorSample(data);
        return TextFileFormat::ReadEncodingError;
    }
    return TextFileFormat::ReadSuccess;
}

/*!
    \brief Read text file into a list of strings.
*/

TextFileFormat::ReadResult
    TextFileFormat::readFile(const QString &fileName, const QTextCodec *defaultCodec,
                             QStringList *plainTextList, TextFileFormat *format, QString *errorString,
                             QByteArray *decodingErrorSample /* = 0 */)
{
    const TextFileFormat::ReadResult result =
        readTextFile(fileName, defaultCodec,
                     plainTextList, format, errorString, decodingErrorSample);
    if (debug)
        qDebug().nospace() << Q_FUNC_INFO << fileName << ' ' << *format
                           << " returns " << result << '/' << plainTextList->size() << " chunks";
    return result;
}

/*!
    \brief Read text file into a string.
*/

TextFileFormat::ReadResult
    TextFileFormat::readFile(const QString &fileName, const QTextCodec *defaultCodec,
                             QString *plainText, TextFileFormat *format, QString *errorString,
                             QByteArray *decodingErrorSample /* = 0 */)
{
    const TextFileFormat::ReadResult result =
        readTextFile(fileName, defaultCodec,
                     plainText, format, errorString, decodingErrorSample);
    if (debug)
        qDebug().nospace() << Q_FUNC_INFO << fileName << ' ' << *format
                           << " returns " << result << '/' << plainText->size() << " characters";
    return result;
}

/*!
    \brief Write out a text file.
*/

bool TextFileFormat::writeFile(const QString &fileName, QString plainText, QString *errorString) const
{
    QTC_ASSERT(codec, return false);

    // Does the user want CRLF? If that is native,
    // let QFile do the work, else manually add.
    QIODevice::OpenMode fileMode = QIODevice::NotOpen;
    if (lineTerminationMode == CRLFLineTerminator) {
        if (NativeLineTerminator == CRLFLineTerminator)
            fileMode |= QIODevice::Text;
        else
            plainText.replace(QLatin1Char('\n'), QLatin1String("\r\n"));
    }

    Utils::FileSaver saver(fileName, fileMode);
    if (saver.hasError()) {
        *errorString = saver.errorString();
        return false;
    }
    if (hasUtf8Bom && codec->name() == "UTF-8")
        saver.write("\xef\xbb\xbf", 3);
    saver.write(codec->fromUnicode(plainText));
    const bool ok = saver.finalize(errorString);
    if (debug)
        qDebug().nospace() << Q_FUNC_INFO << fileName << ' ' << *this <<  ' ' << plainText.size()
                           << " bytes, returns " << ok;
    return ok;
}

} // namespace Utils
