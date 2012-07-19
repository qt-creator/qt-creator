/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef CORE_TEXTDOCUMENT_H
#define CORE_TEXTDOCUMENT_H

#include "idocument.h"

#include <utils/textfileformat.h>

namespace Core {

namespace Internal {
class TextDocumentPrivate;
}

class CORE_EXPORT TextDocument : public IDocument
{
    Q_OBJECT
public:
    typedef Utils::TextFileFormat::ReadResult ReadResult;

    explicit TextDocument(QObject *parent = 0);
    virtual ~TextDocument();

    Utils::TextFileFormat format() const;
    const QTextCodec *codec() const;
    void setCodec(const QTextCodec *);
    void switchUtf8Bom();

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
