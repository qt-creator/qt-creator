// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textdocument.h"

#include "editormanager/editormanager.h"

#include <QDebug>
#include <QTextCodec>

/*!
    \class Core::BaseTextDocument
    \inheaderfile coreplugin/textdocument.h
    \inmodule QtCreator

    \brief The BaseTextDocument class is a very general base class for
    documents that work with text.

    This class contains helper methods for saving and reading text files with encoding and
    line ending settings.

    \sa Utils::TextFileFormat
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
    setLineTerminationMode(Core::EditorManager::defaultLineEnding());
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
    Writes out the contents (\a data) of the text file \a filePath.
    Uses the format obtained from the last read() of the file.

    If an error occurs while writing the file, \a errorMessage is set to the
    error details.

    Returns whether the operation was successful.
*/

bool BaseTextDocument::write(const Utils::FilePath &filePath,
                             const QString &data,
                             QString *errorMessage) const
{
    return write(filePath, format(), data, errorMessage);
}

/*!
    Writes out the contents (\a data) of the text file \a filePath.
    Uses the custom format \a format.

    If an error occurs while writing the file, \a errorMessage is set to the
    error details.

    Returns whether the operation was successful.
*/

bool BaseTextDocument::write(const Utils::FilePath &filePath,
                             const Utils::TextFileFormat &format,
                             const QString &data,
                             QString *errorMessage) const
{
    if (debug)
        qDebug() << Q_FUNC_INFO << this << filePath;
    return format.writeFile(filePath, data, errorMessage);
}

void BaseTextDocument::setSupportsUtf8Bom(bool value)
{
    d->m_supportsUtf8Bom = value;
}

void BaseTextDocument::setLineTerminationMode(Utils::TextFileFormat::LineTerminationMode mode)
{
    d->m_format.lineTerminationMode = mode;
}

/*!
    Autodetects file format and reads the text file specified by \a filePath
    into a list of strings specified by \a plainTextList.

    If an error occurs while writing the file, \a errorString is set to the
    error details.

    Returns whether the operation was successful.
*/

BaseTextDocument::ReadResult BaseTextDocument::read(const Utils::FilePath &filePath,
                                                    QStringList *plainTextList,
                                                    QString *errorString)
{
    d->m_readResult = Utils::TextFileFormat::readFile(filePath,
                                                      codec(),
                                                      plainTextList,
                                                      &d->m_format,
                                                      errorString,
                                                      &d->m_decodingErrorSample);
    return d->m_readResult;
}

/*!
    Autodetects file format and reads the text file specified by \a filePath
    into \a plainText.

    If an error occurs while writing the file, \a errorString is set to the
    error details.

    Returns whether the operation was successful.
*/

BaseTextDocument::ReadResult BaseTextDocument::read(const Utils::FilePath &filePath,
                                                    QString *plainText,
                                                    QString *errorString)
{
    d->m_readResult = Utils::TextFileFormat::readFile(filePath,
                                                      codec(),
                                                      plainText,
                                                      &d->m_format,
                                                      errorString,
                                                      &d->m_decodingErrorSample);
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
    if (supportsCodec(codec))
        d->m_format.codec = codec;
}

bool BaseTextDocument::supportsCodec(const QTextCodec *) const
{
    return true;
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

Utils::TextFileFormat::LineTerminationMode BaseTextDocument::lineTerminationMode() const
{
    return d->m_format.lineTerminationMode;
}

/*!
    Returns the format obtained from the last call to read().
*/

Utils::TextFileFormat BaseTextDocument::format() const
{
    return d->m_format;
}

} // namespace Core
