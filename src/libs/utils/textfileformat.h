// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "result.h"
#include "textcodec.h"
#include "utils_global.h"

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
    };

    TextFileFormat();

    void detectFromData(const QByteArray &data);

    bool decode(const QByteArray &data, QString *target) const;

    ReadResult readFile(const FilePath &filePath, const TextEncoding &fallbackEncoding);

    static Result<> readFileUtf8(const FilePath &filePath,
                                 const TextEncoding &fallbackEncoding,
                                 QByteArray *plainText);

    Result<> writeFile(const FilePath &filePath, QString plainText) const;

    static QByteArray decodingErrorSample(const QByteArray &data);

    LineTerminationMode lineTerminationMode = NativeLineTerminator;
    bool hasUtf8Bom = false;

    TextEncoding encoding() const;
    void setEncoding(const TextEncoding &encoding);

private:
    TextEncoding m_encoding;
};

} // namespace Utils
