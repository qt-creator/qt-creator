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
#include <QUrl>

#include <functional>
#include <memory>

namespace Utils { class FilePath; }

QT_BEGIN_NAMESPACE
class QDataStream;
class QDateTime;
class QDir;
class QFile;
class QFileInfo;
class QTemporaryFile;
class QTextStream;
class QWidget;

QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug dbg, const Utils::FilePath &c);

// for withNtfsPermissions
#ifdef Q_OS_WIN
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif

QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT FilePath
{
public:
    FilePath();

    static FilePath fromString(const QString &filepath);
    static FilePath fromFileInfo(const QFileInfo &info);
    static FilePath fromStringWithExtension(const QString &filepath, const QString &defaultExtension);
    static FilePath fromUserInput(const QString &filepath);
    static FilePath fromUtf8(const char *filepath, int filepathSize = -1);
    static FilePath fromVariant(const QVariant &variant);

    const QString &toString() const;
    QFileInfo toFileInfo() const;
    QVariant toVariant() const;

    QString toUserOutput() const;
    QString shortNativePath() const;

    QString fileName() const;
    QString fileNameWithPathComponents(int pathComponents) const;
    bool exists() const;
    bool isWritablePath() const;

    FilePath parentDir() const;
    FilePath absolutePath() const;

    bool operator==(const FilePath &other) const;
    bool operator!=(const FilePath &other) const;
    bool operator<(const FilePath &other) const;
    bool operator<=(const FilePath &other) const;
    bool operator>(const FilePath &other) const;
    bool operator>=(const FilePath &other) const;
    FilePath operator+(const QString &s) const;

    bool isChildOf(const FilePath &s) const;
    bool isChildOf(const QDir &dir) const;
    bool startsWith(const QString &s) const;
    bool endsWith(const QString &s) const;

    bool isDir() const;
    bool isNewerThan(const QDateTime &timeStamp) const;

    FilePath relativeChildPath(const FilePath &parent) const;
    FilePath pathAppended(const QString &str) const;
    FilePath stringAppended(const QString &str) const;
    FilePath resolvePath(const QString &fileName) const;

    FilePath canonicalPath() const;

    FilePath operator/(const QString &str) const;

    void clear() { m_data.clear(); }
    bool isEmpty() const { return m_data.isEmpty(); }

    uint hash(uint seed) const;

    // NOTE: FilePath operations on FilePath created from URL currenly
    // do not work except for .toVariant() and .toUrl().
    static FilePath fromUrl(const QUrl &url);
    QUrl toUrl() const;

private:
    QString m_data;
    QUrl m_url;
};

QTCREATOR_UTILS_EXPORT QTextStream &operator<<(QTextStream &s, const FilePath &fn);

using FilePaths = QList<FilePath>;

class QTCREATOR_UTILS_EXPORT CommandLine
{
public:
    enum RawType { Raw };

    CommandLine();
    explicit CommandLine(const QString &executable);
    explicit CommandLine(const FilePath &executable);
    CommandLine(const QString &exe, const QStringList &args);
    CommandLine(const FilePath &exe, const QStringList &args);
    CommandLine(const FilePath &exe, const QString &unparsedArgs, RawType);

    void addArg(const QString &arg, OsType osType = HostOsInfo::hostOs());
    void addArgs(const QStringList &inArgs, OsType osType = HostOsInfo::hostOs());

    void addArgs(const QString &inArgs, RawType);

    QString toUserOutput() const;

    FilePath executable() const { return m_executable; }
    QString arguments() const { return m_arguments; }
    QStringList splitArguments(OsType osType = HostOsInfo::hostOs()) const;

private:
    FilePath m_executable;
    QString m_arguments;
};

class QTCREATOR_UTILS_EXPORT FileUtils {
public:
#ifdef QT_GUI_LIB
    class QTCREATOR_UTILS_EXPORT CopyAskingForOverwrite
    {
    public:
        CopyAskingForOverwrite(QWidget *dialogParent);
        bool operator()(const QFileInfo &src, const QFileInfo &dest, QString *error);
        QStringList files() const;

    private:
        QWidget *m_parent;
        QStringList m_files;
        bool m_overwriteAll = false;
        bool m_skipAll = false;
    };
#endif // QT_GUI_LIB

    static bool removeRecursively(const FilePath &filePath, QString *error = nullptr);
    static bool copyRecursively(
            const FilePath &srcFilePath, const FilePath &tgtFilePath, QString *error = nullptr,
            const std::function<bool (QFileInfo, QFileInfo, QString *)> &copyHelper = nullptr);
    static FilePath resolveSymlinks(const FilePath &path);
    static QString fileSystemFriendlyName(const QString &name);
    static int indexOfQmakeUnfriendly(const QString &name, int startpos = 0);
    static QString qmakeFriendlyName(const QString &name);
    static bool makeWritable(const FilePath &path);
    static QString normalizePathName(const QString &name);

    static bool isRelativePath(const QString &fileName);
    static bool isAbsolutePath(const QString &fileName) { return !isRelativePath(fileName); }
    static FilePath commonPath(const FilePath &oldCommonPath, const FilePath &fileName);
    static QByteArray fileId(const FilePath &fileName);
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
    bool m_hasError = false;

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
    bool m_autoRemove = true;
};

inline uint qHash(const Utils::FilePath &a, uint seed = 0) { return a.hash(seed); }

} // namespace Utils

namespace std {
template<> struct QTCREATOR_UTILS_EXPORT hash<Utils::FilePath>
{
    using argument_type = Utils::FilePath;
    using result_type = size_t;
    result_type operator()(const argument_type &fn) const;
};
} // namespace std

Q_DECLARE_METATYPE(Utils::FilePath)
Q_DECLARE_METATYPE(Utils::CommandLine)
