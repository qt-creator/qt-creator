/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "fileutils.h"
#include "savefile.h"

#include "qtcassert.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>
#include <QtCore/QDataStream>
#include <QtCore/QTextStream>
#include <QtCore/QXmlStreamWriter>
#include <QtGui/QMessageBox>

namespace Utils {

QByteArray FileReader::fetchQrc(const QString &fileName)
{
    QTC_ASSERT(fileName.startsWith(QLatin1Char(':')), return QByteArray())
    QFile file(fileName);
    bool ok = file.open(QIODevice::ReadOnly);
    QTC_ASSERT(ok, qWarning() << fileName << "not there!"; return QByteArray())
    return file.readAll();
}

bool FileReader::fetch(const QString &fileName, QIODevice::OpenMode mode)
{
    QTC_ASSERT(!(mode & ~(QIODevice::ReadOnly | QIODevice::Text)), return false)

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | mode)) {
        m_errorString = tr("Cannot open %1 for reading: %2").arg(
                QDir::toNativeSeparators(fileName), file.errorString());
        return false;
    }
    m_data = file.readAll();
    if (file.error() != QFile::NoError) {
        m_errorString = tr("Cannot read %1: %2").arg(
                QDir::toNativeSeparators(fileName), file.errorString());
        return false;
    }
    return true;
}

bool FileReader::fetch(const QString &fileName, QIODevice::OpenMode mode, QString *errorString)
{
    if (fetch(fileName, mode))
        return true;
    if (errorString)
        *errorString = m_errorString;
    return false;
}

bool FileReader::fetch(const QString &fileName, QIODevice::OpenMode mode, QWidget *parent)
{
    if (fetch(fileName, mode))
        return true;
    if (parent)
        QMessageBox::critical(parent, tr("File Error"), m_errorString);
    return false;
}


FileSaverBase::FileSaverBase()
    : m_hasError(false)
{
}

FileSaverBase::~FileSaverBase()
{
    delete m_file;
}

bool FileSaverBase::finalize()
{
    m_file->close();
    setResult(m_file->error() == QFile::NoError);
    // We delete the object, so it is really closed even if it is a QTemporaryFile.
    delete m_file;
    m_file = 0;
    return !m_hasError;
}

bool FileSaverBase::finalize(QString *errStr)
{
    if (finalize())
        return true;
    if (errStr)
        *errStr = errorString();
    return false;
}

bool FileSaverBase::finalize(QWidget *parent)
{
    if (finalize())
        return true;
    QMessageBox::critical(parent, tr("File Error"), errorString());
    return false;
}

bool FileSaverBase::write(const char *data, int len)
{
    if (m_hasError)
        return false;
    return setResult(m_file->write(data, len) == len);
}

bool FileSaverBase::write(const QByteArray &bytes)
{
    if (m_hasError)
        return false;
    return setResult(m_file->write(bytes) == bytes.count());
}

bool FileSaverBase::setResult(bool ok)
{
    if (!ok && !m_hasError) {
        m_errorString = tr("Cannot write file %1. Disk full?").arg(
                QDir::toNativeSeparators(m_fileName));
        m_hasError = true;
    }
    return ok;
}

bool FileSaverBase::setResult(QTextStream *stream)
{
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
    stream->flush();
    return setResult(stream->status() == QTextStream::Ok);
#else
    Q_UNUSED(stream)
    return true;
#endif
}

bool FileSaverBase::setResult(QDataStream *stream)
{
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
    stream->flush();
    return setResult(stream->status() == QTextStream::Ok);
#else
    Q_UNUSED(stream)
    return true;
#endif
}

bool FileSaverBase::setResult(QXmlStreamWriter *stream)
{
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
    return setResult(!stream->hasError());
#else
    Q_UNUSED(stream)
    return true;
#endif
}


FileSaver::FileSaver(const QString &filename, QIODevice::OpenMode mode)
{
    m_fileName = filename;
    if (mode & (QIODevice::ReadOnly | QIODevice::Append)) {
        m_file = new QFile(filename);
        m_isSafe = false;
    } else {
        m_file = new SaveFile(filename);
        m_isSafe = true;
    }
    if (!m_file->open(QIODevice::WriteOnly | mode)) {
        QString err = QFile::exists(filename) ?
                tr("Cannot overwrite file %1: %2") : tr("Cannot create file %1: %2");
        m_errorString = err.arg(QDir::toNativeSeparators(filename), m_file->errorString());
        m_hasError = true;
    }
}

bool FileSaver::finalize()
{
    if (!m_isSafe)
        return FileSaverBase::finalize();

    SaveFile *sf = static_cast<SaveFile *>(m_file);
    if (m_hasError)
        sf->rollback();
    else
        setResult(sf->commit());
    delete sf;
    m_file = 0;
    return !m_hasError;
}

TempFileSaver::TempFileSaver(const QString &templ)
    : m_autoRemove(true)
{
    QTemporaryFile *tempFile = new QTemporaryFile();
    if (!templ.isEmpty())
        tempFile->setFileTemplate(templ);
    tempFile->setAutoRemove(false);
    if (!tempFile->open()) {
        m_errorString = tr("Cannot create temporary file in %1: %2").arg(
                QDir::toNativeSeparators(QFileInfo(tempFile->fileTemplate()).absolutePath()),
                tempFile->errorString());
        m_hasError = true;
    }
    m_file = tempFile;
    m_fileName = tempFile->fileName();
}

TempFileSaver::~TempFileSaver()
{
    delete m_file;
    m_file = 0;
    if (m_autoRemove)
        QFile::remove(m_fileName);
}

} // namespace Utils
