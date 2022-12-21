// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "idocument.h"

#include <utils/textfileformat.h>

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
    const QTextCodec *codec() const;
    void setCodec(const QTextCodec *);
    virtual bool supportsCodec(const QTextCodec *) const;
    void switchUtf8Bom();
    bool supportsUtf8Bom() const;
    Utils::TextFileFormat::LineTerminationMode lineTerminationMode() const;

    ReadResult read(const Utils::FilePath &filePath, QStringList *plainTextList, QString *errorString);
    ReadResult read(const Utils::FilePath &filePath, QString *plainText, QString *errorString);

    bool hasDecodingError() const;
    QByteArray decodingErrorSample() const;

    bool write(const Utils::FilePath &filePath, const QString &data, QString *errorMessage) const;
    bool write(const Utils::FilePath &filePath, const Utils::TextFileFormat &format, const QString &data, QString *errorMessage) const;

    void setSupportsUtf8Bom(bool value);
    void setLineTerminationMode(Utils::TextFileFormat::LineTerminationMode mode);

private:
    Internal::TextDocumentPrivate *d;
};

} // namespace Core
