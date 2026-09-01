// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "result.h"
#include "textcodec.h"
#include "utils_global.h"

#include <QList>
#include <QStringList>

namespace Utils {

class FilePath;

class QTCREATOR_UTILS_EXPORT TextFileFormat
{
public:
    enum LineTerminationMode
    {
        LFLineTerminator,
        CRLFLineTerminator,
        NativeLineTerminator =
#if defined (Q_OS_WIN)
        CRLFLineTerminator,
#else
        LFLineTerminator
#endif
    };

    enum ReadResultCode {
        ReadSuccess,
        ReadEncodingError,
        ReadMemoryAllocationError,
        ReadIOError
    };

    class QTCREATOR_UTILS_EXPORT ReadResult
    {
    public:
        ReadResult() = default;
        ReadResult(ReadResultCode code) : code(code) {}
        ReadResult(ReadResultCode code, const QString &error) : code(code), error(error) {}

        ReadResultCode code = ReadSuccess;
        QString content;
        QString error;
        QByteArray decodingErrorSample;
        QByteArray decodingErrorData;
        bool usedDecodingFallback = false;
        // Zero-based byte offset and UTF-16 position of the first decoding error.
        // A value of -1 means that the position could not be determined.
        qsizetype decodingErrorOffset = -1;
        qsizetype decodingErrorPosition = -1;
        QList<qsizetype> decodingErrorOffsets;
    };

    TextFileFormat();

    void detectFromData(const QByteArray &data);

    bool decode(const QByteArray &data, QString *target) const;

    static constexpr qsizetype MaximumDecodingErrorOffsets = 1000;

    enum class DecodingErrorDetails { Skip, Collect };

    // If retryEncoding is valid, retries with it after a decoding error.
    ReadResult readFile(const FilePath &filePath,
                        const TextEncoding &defaultEncoding,
                        const TextEncoding &retryEncoding = {},
                        DecodingErrorDetails errorDetails = DecodingErrorDetails::Skip);

    static Result<> readFileUtf8(const FilePath &filePath,
                                 const TextEncoding &fallbackEncoding,
                                 QByteArray *plainText);

    Result<> writeFile(const FilePath &filePath, QString plainText) const;

    static QByteArray decodingErrorSample(const QByteArray &data);
    static QList<qsizetype> decodedPositionsForByteOffsets(
        const QByteArray &data, const TextEncoding &encoding,
        const QList<qsizetype> &byteOffsets, LineTerminationMode lineTerminationMode);

    LineTerminationMode lineTerminationMode = NativeLineTerminator;
    bool hasUtf8Bom = false;

    TextEncoding encoding() const;
    void setEncoding(const TextEncoding &encoding);

private:
    TextEncoding m_encoding;
};

} // namespace Utils
