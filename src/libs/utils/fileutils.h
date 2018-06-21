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

#include "utils_global.h"

#include "hostosinfo.h"

#include <QCoreApplication>
#include <QXmlStreamWriter> // Mac.
#include <QMetaType>
#include <QStringList>

#include <functional>
#include <memory>

namespace Utils {class FileName; }

QT_BEGIN_NAMESPACE
class QDataStream;
class QDateTime;
class QDir;
class QFile;
class QFileInfo;
class QTemporaryFile;
class QTextStream;
class QWidget;

QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug dbg, const Utils::FileName &c);

// for withNtfsPermissions
#ifdef Q_OS_WIN
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif

QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT FileName : private QString
{
public:
    FileName();
    explicit FileName(const QFileInfo &info);
    QFileInfo toFileInfo() const;
    static FileName fromString(const QString &filename);
    static FileName fromString(const QString &filename, const QString &defaultExtension);
    static FileName fromLatin1(const QByteArray &filename);
    static FileName fromUserInput(const QString &filename);
    static FileName fromUtf8(const char *filename, int filenameSize = -1);
    const QString &toString() const;
    QString toUserOutput() const;
    QString fileName(int pathComponents = 0) const;
    bool exists() const;

    FileName parentDir() const;

    bool operator==(const FileName &other) const;
    bool operator!=(const FileName &other) const;
    bool operator<(const FileName &other) const;
    bool operator<=(const FileName &other) const;
    bool operator>(const FileName &other) const;
    bool operator>=(const FileName &other) const;
    FileName operator+(const QString &s) const;

    bool isChildOf(const FileName &s) const;
    bool isChildOf(const QDir &dir) const;
    bool endsWith(const QString &s) const;

    FileName relativeChildPath(const FileName &parent) const;
    FileName &appendPath(const QString &s);
    FileName &appendString(const QString &str);
    FileName &appendString(QChar str);

    using QString::chop;
    using QString::clear;
    using QString::count;
    using QString::isEmpty;
    using QString::isNull;
    using QString::length;
    using QString::size;
private:
    FileName(const QString &string);
};

QTCREATOR_UTILS_EXPORT QTextStream &operator<<(QTextStream &s, const FileName &fn);

using FileNameList = QList<FileName>;

class QTCREATOR_UTILS_EXPORT FileUtils {
public:
    static bool removeRecursively(const FileName &filePath, QString *error = nullptr);
    static bool copyRecursively(
            const FileName &srcFilePath, const FileName &tgtFilePath, QString *error = nullptr,
            const std::function<bool (QFileInfo, QFileInfo, QString *)> &copyHelper = nullptr);
    static bool isFileNewerThan(const FileName &filePath, const QDateTime &timeStamp);
    static FileName resolveSymlinks(const FileName &path);
    static FileName canonicalPath(const FileName &path);
    static QString shortNativePath(const FileName &path);
    static QString fileSystemFriendlyName(const QString &name);
    static int indexOfQmakeUnfriendly(const QString &name, int startpos = 0);
    static QString qmakeFriendlyName(const QString &name);
    static bool makeWritable(const FileName &path);
    static QString normalizePathName(const QString &name);

    static bool isRelativePath(const QString &fileName);
    static bool isAbsolutePath(const QString &fileName) { return !isRelativePath(fileName); }
    static QString resolvePath(const QString &baseDir, const QString &fileName);
};

// for actually finding out if e.g. directories are writable on Windows
#ifdef Q_OS_WIN

template <typename T>
T withNtfsPermissions(const std::function<T()> &task)
{
    qt_ntfs_permission_lookup++;
    T result = task();
    qt_ntfs_permission_lookup--;
    return result;
}

template <>
QTCREATOR_UTILS_EXPORT void withNtfsPermissions(const std::function<void()> &task);

#else // Q_OS_WIN

template <typename T>
T withNtfsPermissions(const std::function<T()> &task)
{
    return task();
}

#endif // Q_OS_WIN

class QTCREATOR_UTILS_EXPORT FileReader
{
    Q_DECLARE_TR_FUNCTIONS(Utils::FileUtils) // sic!
public:
    static QByteArray fetchQrc(const QString &fileName); // Only for internal resources
    bool fetch(const QString &fileName, QIODevice::OpenMode mode = QIODevice::NotOpen); // QIODevice::ReadOnly is implicit
    bool fetch(const QString &fileName, QIODevice::OpenMode mode, QString *errorString);
    bool fetch(const QString &fileName, QString *errorString)
        { return fetch(fileName, QIODevice::NotOpen, errorString); }
#ifdef QT_GUI_LIB
    bool fetch(const QString &fileName, QIODevice::OpenMode mode, QWidget *parent);
    bool fetch(const QString &fileName, QWidget *parent)
        { return fetch(fileName, QIODevice::NotOpen, parent); }
#endif // QT_GUI_LIB
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
#ifdef QT_GUI_LIB
    bool finalize(QWidget *parent);
#endif

    bool write(const char *data, int len);
    bool write(const QByteArray &bytes);
    bool setResult(QTextStream *stream);
    bool setResult(QDataStream *stream);
    bool setResult(QXmlStreamWriter *stream);
    bool setResult(bool ok);

    QFile *file() { return m_file.get(); }

protected:
    std::unique_ptr<QFile> m_file;
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
    // QIODevice::WriteOnly is implicit
    explicit FileSaver(const QString &filename, QIODevice::OpenMode mode = QIODevice::NotOpen);

    bool finalize() override;
    using FileSaverBase::finalize;

private:
    bool m_isSafe;
};

class QTCREATOR_UTILS_EXPORT TempFileSaver : public FileSaverBase
{
    Q_DECLARE_TR_FUNCTIONS(Utils::FileUtils) // sic!
public:
    explicit TempFileSaver(const QString &templ = QString());
    ~TempFileSaver() override;

    void setAutoRemove(bool on) { m_autoRemove = on; }

private:
    bool m_autoRemove;
};

} // namespace Utils

QT_BEGIN_NAMESPACE
QTCREATOR_UTILS_EXPORT uint qHash(const Utils::FileName &a);
QT_END_NAMESPACE

namespace std {
template<> struct hash<Utils::FileName>
{
    using argument_type = Utils::FileName;
    using result_type = size_t;
    result_type operator()(const argument_type &fn) const
    {
        if (Utils::HostOsInfo::fileNameCaseSensitivity() == Qt::CaseInsensitive)
            return hash<string>()(fn.toString().toUpper().toStdString());
        return hash<string>()(fn.toString().toStdString());
    }
};
} // namespace std

Q_DECLARE_METATYPE(Utils::FileName)
