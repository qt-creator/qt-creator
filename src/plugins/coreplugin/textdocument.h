// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "idocument.h"

#include <utils/textfileformat.h>

#include <QList>

namespace Core {

namespace Internal { class TextDocumentPrivate; }

class CORE_EXPORT BaseTextDocument : public IDocument
{
    Q_OBJECT

public:
    using ReadResult = Utils::TextFileFormat::ReadResult;

    explicit BaseTextDocument(QObject *parent = nullptr);
    ~BaseTextDocument() override;

    Utils::TextFileFormat format() const;

    Utils::TextEncoding encoding() const;
    void setEncoding(const Utils::TextEncoding &encoding);
    bool supportsEncoding(const Utils::TextEncoding &encoding) const;
    void setSupportedEncodings(const QList<Utils::TextEncoding> &encodings);

    void switchUtf8Bom();
    bool supportsUtf8Bom() const;

    Utils::TextFileFormat::LineTerminationMode lineTerminationMode() const;

    enum class DecodingFallback { Allowed, Disabled };

    /*! Reads a filePath using the current encoding.

        If a fallback is Allowed and fallback decoding is enabled for this
        document, retries a failed decode with the configured fallback encoding.
        Fallback decoding is disabled by default for BaseTextDocument;
        TextEditor::TextDocument enables it by default.
    */
    ReadResult read(const Utils::FilePath &filePath,
                    DecodingFallback fallback = DecodingFallback::Allowed);
    virtual QString plainText() const;

    bool hasDecodingError() const;
    //! Returns whether the last read succeeded using the configured fallback encoding.
    bool decodedWithFallback() const;
    //! Returns the encoding that failed when the last read used fallback decoding.
    Utils::TextEncoding failedPrimaryEncoding() const;
    QByteArray decodingErrorSample() const;
    QByteArray decodingErrorData() const;
    qsizetype decodingErrorOffset() const;
    qsizetype decodingErrorPosition() const;
    QList<qsizetype> decodingErrorOffsets() const;
    //! Returns whether reads may retry with the configured fallback encoding.
    bool isDecodingFallbackEnabled() const;
    //! Enables or disables retrying reads with the configured fallback encoding.
    void setDecodingFallbackEnabled(bool enabled);

    Utils::Result<> write(const Utils::FilePath &filePath, const QString &data) const;
    Utils::Result<> write(const Utils::FilePath &filePath, const Utils::TextFileFormat &format, const QString &data) const;

    void setSupportsUtf8Bom(bool value);
    void setLineTerminationMode(Utils::TextFileFormat::LineTerminationMode mode);

signals:
    //! Emitted when the last read's fallback state or failed primary encoding changes.
    void decodingFallbackChanged();

protected:
    void restoreDecodingFallbackState(const Utils::TextEncoding &failedPrimaryEncoding);

    // Collect byte offsets and retain the original bytes for editor diagnostics.
    // General BaseTextDocument users skip this additional work by default.
    void setCollectDecodingErrorDetails(bool collect);

private:
    Internal::TextDocumentPrivate *d;
};

} // namespace Core
