/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CORE_TEXTDOCUMENT_H
#define CORE_TEXTDOCUMENT_H

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
    virtual bool supportsUtf8Bom() { return true; }

    ReadResult read(const QString &fileName, QStringList *plainTextList, QString *errorString);
    ReadResult read(const QString &fileName, QString *plainText, QString *errorString);

    bool hasDecodingError() const;
    QByteArray decodingErrorSample() const;

    bool write(const QString &fileName, const QString &data, QString *errorMessage) const;
    bool write(const QString &fileName, const Utils::TextFileFormat &format, const QString &data, QString *errorMessage) const;

private:
    Internal::TextDocumentPrivate *d;
};

} // namespace Core

#endif // CORE_TEXTDOCUMENT_H
