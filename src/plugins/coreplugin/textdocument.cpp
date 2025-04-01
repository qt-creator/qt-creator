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

using namespace Utils;

namespace Core {

namespace Internal {

class TextDocumentPrivate
{
public:
    TextFileFormat m_format;
    TextFileFormat::ReadResult m_readResult = TextFileFormat::ReadSuccess;
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
    return d->m_readResult == TextFileFormat::ReadEncodingError;
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

Result BaseTextDocument::write(const FilePath &filePath, const QString &data) const
{
    return write(filePath, format(), data);
}

/*!
    Writes out the contents (\a data) of the text file \a filePath.
    Uses the custom format \a format.

    If an error occurs while writing the file, \a errorMessage is set to the
    error details.

    Returns whether the operation was successful.
*/

Result BaseTextDocument::write(const FilePath &filePath,
                               const TextFileFormat &format,
                               const QString &data) const
{
    if (debug)
        qDebug() << Q_FUNC_INFO << this << filePath;
    return format.writeFile(filePath, data);
}

void BaseTextDocument::setSupportsUtf8Bom(bool value)
{
    d->m_supportsUtf8Bom = value;
}

void BaseTextDocument::setLineTerminationMode(TextFileFormat::LineTerminationMode mode)
{
    d->m_format.lineTerminationMode = mode;
}

bool BaseTextDocument::isUtf8Codec(const QByteArray &name)
{
    static const auto utf8Codecs = []() -> QList<QByteArray> {
        QTextCodec *codec = QTextCodec::codecForName("UTF-8");
        if (QTC_GUARD(codec))
            return QList<QByteArray>{codec->name()} + codec->aliases();
        return {"UTF-8"};
    }();

    return utf8Codecs.contains(name);
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
    return d->m_format.codec();
}

QByteArray BaseTextDocument::codecName() const
{
    return d->m_format.codecName();
}

void BaseTextDocument::setCodec(const QTextCodec *codec)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << this << (codec ? codec->name() : QByteArray());
    if (supportsCodec(codec ? codec->name() : QByteArray()))
        d->m_format.setCodec(codec);
}

void BaseTextDocument::setCodec(const QByteArray &name)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << this << name;
    if (supportsCodec(name))
        d->m_format.setCodecName(name);
}

bool BaseTextDocument::supportsCodec(const QByteArray &) const
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

TextFileFormat::LineTerminationMode BaseTextDocument::lineTerminationMode() const
{
    return d->m_format.lineTerminationMode;
}

/*!
    Returns the format obtained from the last call to read().
*/

TextFileFormat BaseTextDocument::format() const
{
    return d->m_format;
}

} // namespace Core
