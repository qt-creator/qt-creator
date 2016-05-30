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

#include "textdocument.h"
#include <coreplugin/editormanager/editormanager.h>

#include <QDebug>
#include <QTextCodec>

/*!
    \class Core::TextDocument
    \brief The TextDocument class is a very general base class for documents that work with text.

    This class contains helper methods for saving and reading text files with encoding and
    line ending settings.

    \sa Utils::TextFileUtils
*/

enum { debug = 0 };

namespace Core {

namespace Internal {
class TextDocumentPrivate
{
public:
    Utils::TextFileFormat m_format;
    Utils::TextFileFormat::ReadResult m_readResult = Utils::TextFileFormat::ReadSuccess;
    QByteArray m_decodingErrorSample;
    bool m_supportsUtf8Bom = true;
};

} // namespace Internal

BaseTextDocument::BaseTextDocument(QObject *parent) :
    IDocument(parent), d(new Internal::TextDocumentPrivate)
{
    setCodec(Core::EditorManager::defaultTextCodec());
}

BaseTextDocument::~BaseTextDocument()
{
    delete d;
}

bool BaseTextDocument::hasDecodingError() const
{
    return d->m_readResult == Utils::TextFileFormat::ReadEncodingError;
}

QByteArray BaseTextDocument::decodingErrorSample() const
{
    return d->m_decodingErrorSample;
}

/*!
    Writes out text using the format obtained from the last read.
*/

bool BaseTextDocument::write(const QString &fileName, const QString &data, QString *errorMessage) const
{
    return write(fileName, format(), data, errorMessage);
}

/*!
    Writes out text using a custom \a format.
*/

bool BaseTextDocument::write(const QString &fileName, const Utils::TextFileFormat &format, const QString &data, QString *errorMessage) const
{
    if (debug)
        qDebug() << Q_FUNC_INFO << this << fileName;
    return format.writeFile(fileName, data, errorMessage);
}

void BaseTextDocument::setSupportsUtf8Bom(bool value)
{
    d->m_supportsUtf8Bom = value;
}

/*!
    Autodetects format and reads in the text file specified by \a fileName.
*/

BaseTextDocument::ReadResult BaseTextDocument::read(const QString &fileName, QStringList *plainTextList, QString *errorString)
{
    d->m_readResult =
        Utils::TextFileFormat::readFile(fileName, codec(),
                                        plainTextList, &d->m_format, errorString, &d->m_decodingErrorSample);
    return d->m_readResult;
}

/*!
    Autodetects format and reads in the text file specified by \a fileName.
*/

BaseTextDocument::ReadResult BaseTextDocument::read(const QString &fileName, QString *plainText, QString *errorString)
{
    d->m_readResult =
        Utils::TextFileFormat::readFile(fileName, codec(),
                                        plainText, &d->m_format, errorString, &d->m_decodingErrorSample);
    return d->m_readResult;
}

const QTextCodec *BaseTextDocument::codec() const
{
    return d->m_format.codec;
}

void BaseTextDocument::setCodec(const QTextCodec *codec)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << this << (codec ? codec->name() : QByteArray());
    d->m_format.codec = codec;
}

void BaseTextDocument::switchUtf8Bom()
{
    if (debug)
        qDebug() << Q_FUNC_INFO << this << "UTF-8 BOM: " << !d->m_format.hasUtf8Bom;
    d->m_format.hasUtf8Bom = !d->m_format.hasUtf8Bom;
}

bool BaseTextDocument::supportsUtf8Bom() const
{
    return d->m_supportsUtf8Bom;
}

/*!
    Returns the format obtained from the last call to \c read().
*/

Utils::TextFileFormat BaseTextDocument::format() const
{
    return d->m_format;
}

} // namespace Core
