/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "idocument.h"

#include <utils/textfileformat.h>

namespace Core {

namespace Internal { class TextDocumentPrivate; }

class CORE_EXPORT BaseTextDocument : public IDocument
{
    Q_OBJECT

public:
    typedef Utils::TextFileFormat::ReadResult ReadResult;

    explicit BaseTextDocument(QObject *parent = 0);
    ~BaseTextDocument() override;

    Utils::TextFileFormat format() const;
    const QTextCodec *codec() const;
    void setCodec(const QTextCodec *);
    void switchUtf8Bom();
    bool supportsUtf8Bom() const;

    ReadResult read(const QString &fileName, QStringList *plainTextList, QString *errorString);
    ReadResult read(const QString &fileName, QString *plainText, QString *errorString);

    bool hasDecodingError() const;
    QByteArray decodingErrorSample() const;

    bool write(const QString &fileName, const QString &data, QString *errorMessage) const;
    bool write(const QString &fileName, const Utils::TextFileFormat &format, const QString &data, QString *errorMessage) const;

    void setSupportsUtf8Bom(bool value);

private:
    Internal::TextDocumentPrivate *d;
};

} // namespace Core
