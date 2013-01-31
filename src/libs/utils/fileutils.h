/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FILEUTILS_H
#define FILEUTILS_H

#include "utils_global.h"

#include <QCoreApplication>
#include <QIODevice>
#include <QXmlStreamWriter> // Mac.
#include <QFileInfo>
#include <QMetaType>

QT_BEGIN_NAMESPACE
class QFile;
class QTemporaryFile;
class QWidget;
class QTextStream;
class QDataStream;
class QDateTime;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT FileName : private QString
{
public:
    FileName();
    explicit FileName(const QFileInfo &info);
    QFileInfo toFileInfo() const;
    static FileName fromString(const QString &filename);
    static FileName fromUserInput(const QString &filename);
    QString toString() const;
    QString toUserOutput() const;

    FileName parentDir() const;

    bool operator==(const FileName &other) const;
    bool operator!=(const FileName &other) const;
    bool operator<(const FileName &other) const;
    bool operator<=(const FileName &other) const;
    bool operator>(const FileName &other) const;
    bool operator>=(const FileName &other) const;

    bool isChildOf(const FileName &s) const;
    bool isChildOf(const QDir &dir) const;
    bool endsWith(const QString &s) const;

    Utils::FileName relativeChildPath(const FileName &parent) const;
    Utils::FileName &appendPath(const QString &s);
    Utils::FileName &append(const QString &str);
    Utils::FileName &append(QChar str);

    using QString::size;
    using QString::count;
    using QString::length;
    using QString::isEmpty;
    using QString::isNull;
    using QString::clear;
private:
    FileName(const QString &string);
};

class QTCREATOR_UTILS_EXPORT FileUtils {
public:
    static bool removeRecursively(const FileName &filePath, QString *error = 0);
    static bool copyRecursively(const FileName &srcFilePath, const FileName &tgtFilePath,
                                QString *error = 0);
    static bool isFileNewerThan(const FileName &filePath, const QDateTime &timeStamp);
    static FileName resolveSymlinks(const FileName &path);
    static QString shortNativePath(const FileName &path);
};

class QTCREATOR_UTILS_EXPORT FileReader
{
    Q_DECLARE_TR_FUNCTIONS(Utils::FileUtils) // sic!
public:
    static QByteArray fetchQrc(const QString &fileName); // Only for internal resources
    bool fetch(const QString &fileName, QIODevice::OpenMode mode = QIODevice::NotOpen); // QIODevice::ReadOnly is implicit
    bool fetch(const QString &fileName, QIODevice::OpenMode mode, QString *errorString);
    bool fetch(const QString &fileName, QString *errorString)
        { return fetch(fileName, QIODevice::NotOpen, errorString); }
    bool fetch(const QString &fileName, QIODevice::OpenMode mode, QWidget *parent);
    bool fetch(const QString &fileName, QWidget *parent)
        { return fetch(fileName, QIODevice::NotOpen, parent); }
    const QByteArray &data() const { return m_data; }
    const QString &errorString() const { return m_errorString; }
private:
    QByteArray m_data;
    QString m_errorString;
};

class QTCREATOR_UTILS_EXPORT FileSaverBase
{
    Q_DECLARE_TR_FUNCTIONS(Utils::FileUtils) // sic!
public:
    FileSaverBase();
    virtual ~FileSaverBase();

    QString fileName() const { return m_fileName; }
    bool hasError() const { return m_hasError; }
    QString errorString() const { return m_errorString; }
    virtual bool finalize();
    bool finalize(QString *errStr);
    bool finalize(QWidget *parent);

    bool write(const char *data, int len);
    bool write(const QByteArray &bytes);
    bool setResult(QTextStream *stream);
    bool setResult(QDataStream *stream);
    bool setResult(QXmlStreamWriter *stream);
    bool setResult(bool ok);

protected:
    QFile *m_file;
    QString m_fileName;
    QString m_errorString;
    bool m_hasError;

private:
    Q_DISABLE_COPY(FileSaverBase)
};

class QTCREATOR_UTILS_EXPORT FileSaver : public FileSaverBase
{
    Q_DECLARE_TR_FUNCTIONS(Utils::FileUtils) // sic!
public:
    explicit FileSaver(const QString &filename, QIODevice::OpenMode mode = QIODevice::NotOpen); // QIODevice::WriteOnly is implicit

    virtual bool finalize();
    using FileSaverBase::finalize;
    QFile *file() { return m_file; }

private:
    bool m_isSafe;
};

class QTCREATOR_UTILS_EXPORT TempFileSaver : public FileSaverBase
{
    Q_DECLARE_TR_FUNCTIONS(Utils::FileUtils) // sic!
public:
    explicit TempFileSaver(const QString &templ = QString());
    ~TempFileSaver();

    QTemporaryFile *file() { return reinterpret_cast<QTemporaryFile *>(m_file); }

    void setAutoRemove(bool on) { m_autoRemove = on; }

private:
    bool m_autoRemove;
};

} // namespace Utils

QT_BEGIN_NAMESPACE
QTCREATOR_UTILS_EXPORT uint qHash(const Utils::FileName &a);
QT_END_NAMESPACE

Q_DECLARE_METATYPE(Utils::FileName)

#endif // FILEUTILS_H
