// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "osspecificaspects.h"

#include <QDir>
#include <QDirIterator>
#include <QMetaType>

#include <functional>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE
class QDateTime;
class QDebug;
class QFileInfo;
class QUrl;
QT_END_NAMESPACE

class tst_fileutils; // This becomes a friend of Utils::FilePath for testing private methods.

namespace Utils {

class Environment;
class EnvironmentChange;

class QTCREATOR_UTILS_EXPORT FileFilter
{
public:
    FileFilter(const QStringList &nameFilters,
                   const QDir::Filters fileFilters = QDir::NoFilter,
                   const QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags);

    const QStringList nameFilters;
    const QDir::Filters fileFilters = QDir::NoFilter;
    const QDirIterator::IteratorFlags iteratorFlags = QDirIterator::NoIteratorFlags;
};

using FilePaths = QList<class FilePath>;

class QTCREATOR_UTILS_EXPORT FilePath
{
public:
    FilePath();

    template <size_t N> FilePath(const char (&literal)[N]) { setFromString(literal); }

    [[nodiscard]] static FilePath fromString(const QString &filepath);
    [[nodiscard]] static FilePath fromStringWithExtension(const QString &filepath, const QString &defaultExtension);
    [[nodiscard]] static FilePath fromUserInput(const QString &filepath);
    [[nodiscard]] static FilePath fromUtf8(const char *filepath, int filepathSize = -1);
    [[nodiscard]] static FilePath fromVariant(const QVariant &variant);
    [[nodiscard]] static FilePath fromUrl(const QUrl &url);
    [[nodiscard]] static FilePath fromParts(const QStringView scheme, const QStringView host, const QStringView path);

    [[nodiscard]] static FilePath currentWorkingPath();

    QString toUserOutput() const;
    QString toString() const;
    QVariant toVariant() const;
    QUrl toUrl() const;

    QStringView scheme() const;
    QStringView host() const;
    QString path() const;

    void setParts(const QStringView scheme, const QStringView host, const QStringView path);

    QString fileName() const;
    QString fileNameWithPathComponents(int pathComponents) const;

    QString baseName() const;
    QString completeBaseName() const;
    QString suffix() const;
    QString completeSuffix() const;

    [[nodiscard]] FilePath pathAppended(const QString &str) const;
    [[nodiscard]] FilePath stringAppended(const QString &str) const;
    bool startsWith(const QString &s) const;
    bool endsWith(const QString &s) const;

    bool exists() const;

    FilePath parentDir() const;
    bool isChildOf(const FilePath &s) const;

    bool isWritableDir() const;
    bool isWritableFile() const;
    bool ensureWritableDir() const;
    bool ensureExistingFile() const;
    bool isExecutableFile() const;
    bool isReadableFile() const;
    bool isReadableDir() const;
    bool isRelativePath() const;
    bool isAbsolutePath() const { return !isRelativePath(); }
    bool isFile() const;
    bool isDir() const;
    bool isRootPath() const;
    bool isNewerThan(const QDateTime &timeStamp) const;
    QDateTime lastModified() const;
    QFile::Permissions permissions() const;
    bool setPermissions(QFile::Permissions permissions) const;
    OsType osType() const;
    bool removeFile() const;
    bool removeRecursively(QString *error = nullptr) const;
    bool copyFile(const FilePath &target) const;
    bool renameFile(const FilePath &target) const;
    qint64 fileSize() const;
    qint64 bytesAvailable() const;
    bool createDir() const;
    FilePaths dirEntries(const FileFilter &filter, QDir::SortFlags sort = QDir::NoSort) const;
    FilePaths dirEntries(QDir::Filters filters) const;
    std::optional<QByteArray> fileContents(qint64 maxSize = -1, qint64 offset = 0) const;
    bool writeFileContents(const QByteArray &data) const;

    bool operator==(const FilePath &other) const;
    bool operator!=(const FilePath &other) const;
    bool operator<(const FilePath &other) const;
    bool operator<=(const FilePath &other) const;
    bool operator>(const FilePath &other) const;
    bool operator>=(const FilePath &other) const;
    [[nodiscard]] FilePath operator+(const QString &s) const;
    [[nodiscard]] FilePath operator/(const QString &str) const;

    Qt::CaseSensitivity caseSensitivity() const;

    void clear();
    bool isEmpty() const;

    size_t hash(uint seed) const;

    [[nodiscard]] FilePath absoluteFilePath() const;
    [[nodiscard]] FilePath absolutePath() const;
    [[nodiscard]] FilePath resolvePath(const FilePath &tail) const;
    [[nodiscard]] FilePath resolvePath(const QString &tail) const;
    [[nodiscard]] FilePath cleanPath() const;
    [[nodiscard]] FilePath canonicalPath() const;
    [[nodiscard]] FilePath symLinkTarget() const;
    [[nodiscard]] FilePath resolveSymlinks() const;
    [[nodiscard]] FilePath withExecutableSuffix() const;
    [[nodiscard]] FilePath relativeChildPath(const FilePath &parent) const;
    [[nodiscard]] FilePath relativePath(const FilePath &anchor) const;
    [[nodiscard]] FilePath searchInDirectories(const FilePaths &dirs) const;
    [[nodiscard]] Environment deviceEnvironment() const;
    [[nodiscard]] FilePath onDevice(const FilePath &deviceTemplate) const;
    [[nodiscard]] FilePath withNewPath(const QString &newPath) const;
    void iterateDirectory(const std::function<bool(const FilePath &item)> &callBack,
                          const FileFilter &filter) const;
    static void iterateDirectories(const FilePaths &dirs,
                                   const std::function<bool(const FilePath &item)> &callBack,
                                   const FileFilter &filter);

    enum PathAmending { AppendToPath, PrependToPath };
    [[nodiscard]] FilePath searchInPath(const FilePaths &additionalDirs = {},
                                        PathAmending = AppendToPath) const;

    // makes sure that capitalization of directories is canonical
    // on Windows and macOS. This is rarely needed.
    [[nodiscard]] FilePath normalizedPathName() const;

    QString displayName(const QString &args = {}) const;
    QString nativePath() const;
    QString shortNativePath() const;
    bool startsWithDriveLetter() const;

    static QString formatFilePaths(const FilePaths &files, const QString &separator);
    static void removeDuplicates(FilePaths &files);
    static void sort(FilePaths &files);

    // Asynchronous interface
    template <class ...Args> using Continuation = std::function<void(Args...)>;
    void asyncCopyFile(const Continuation<bool> &cont, const FilePath &target) const;
    void asyncFileContents(const Continuation<const std::optional<QByteArray> &> &cont,
                           qint64 maxSize = -1,
                           qint64 offset = 0) const;
    void asyncWriteFileContents(const Continuation<bool> &cont, const QByteArray &data) const;

    // Prefer not to use
    // Using needsDevice() in "user" code is likely to result in code that
    // makes a local/remote distinction which should be avoided in general.
    // There are usually other means available. E.g. distinguishing based
    // on FilePath::osType().
    bool needsDevice() const;

    bool isSameDevice(const FilePath &other) const;

    [[nodiscard]] QFileInfo toFileInfo() const;
    [[nodiscard]] static FilePath fromFileInfo(const QFileInfo &info);

    // Support for FSEngine. Do not use unless needed.
    enum class SpecialPathComponent {
        RootName,
        RootPath,
        DeviceRootName,
        DeviceRootPath,
    };

    [[nodiscard]] static QString specialPath(SpecialPathComponent component);
    [[nodiscard]] static FilePath specialFilePath(SpecialPathComponent component);

    [[nodiscard]] bool ensureReachable(const FilePath &other) const;

    [[nodiscard]] QString toFSPathString() const;

    [[nodiscard]] static int rootLength(const QStringView path); // Assumes no scheme and host
    [[nodiscard]] static int schemeAndHostLength(const QStringView path);

private:
    friend class ::tst_fileutils;
    static QString calcRelativePath(const QString &absolutePath, const QString &absoluteAnchorPath);
    void setPath(QStringView path);
    void setFromString(const QString &filepath);

    [[nodiscard]] QString mapToDevicePath() const;
    [[nodiscard]] QString encodedHost() const;

    QString m_scheme;
    QString m_host; // May contain raw slashes.
    QString m_path; // Includes the root bits
};

inline size_t qHash(const Utils::FilePath &a, uint seed = 0)
{
    return a.hash(seed);
}

class QTCREATOR_UTILS_EXPORT DeviceFileHooks
{
public:
    static DeviceFileHooks &instance();

    std::function<bool(const FilePath &)> isExecutableFile;
    std::function<bool(const FilePath &)> isReadableFile;
    std::function<bool(const FilePath &)> isReadableDir;
    std::function<bool(const FilePath &)> isWritableDir;
    std::function<bool(const FilePath &)> isWritableFile;
    std::function<bool(const FilePath &)> isFile;
    std::function<bool(const FilePath &)> isDir;
    std::function<bool(const FilePath &)> ensureWritableDir;
    std::function<bool(const FilePath &)> ensureExistingFile;
    std::function<bool(const FilePath &)> createDir;
    std::function<bool(const FilePath &)> exists;
    std::function<bool(const FilePath &)> removeFile;
    std::function<bool(const FilePath &)> removeRecursively;
    std::function<bool(const FilePath &, const FilePath &)> copyFile;
    std::function<bool(const FilePath &, const FilePath &)> renameFile;
    std::function<FilePath(const FilePath &, const FilePaths &)> searchInPath;
    std::function<FilePath(const FilePath &)> symLinkTarget;
    std::function<QString(const FilePath &)> mapToDevicePath;
    std::function<void(const FilePath &,
                       const std::function<bool(const FilePath &)> &, // Abort on 'false' return.
                       const FileFilter &)> iterateDirectory;
    std::function<std::optional<QByteArray>(const FilePath &, qint64, qint64)> fileContents;
    std::function<bool(const FilePath &, const QByteArray &)> writeFileContents;
    std::function<QDateTime(const FilePath &)> lastModified;
    std::function<QFile::Permissions(const FilePath &)> permissions;
    std::function<bool(const FilePath &, QFile::Permissions)> setPermissions;
    std::function<OsType(const FilePath &)> osType;
    std::function<Environment(const FilePath &)> environment;
    std::function<qint64(const FilePath &)> fileSize;
    std::function<qint64(const FilePath &)> bytesAvailable;
    std::function<QString(const FilePath &)> deviceDisplayName;
    std::function<bool(const FilePath &, const FilePath &)> isSameDevice;


    template <class ...Args> using Continuation = std::function<void(Args...)>;
    std::function<void(const Continuation<bool> &, const FilePath &, const FilePath &)> asyncCopyFile;
    std::function<void(
        const Continuation<const std::optional<QByteArray> &> &, const FilePath &, qint64, qint64)>
        asyncFileContents;
    std::function<void(const Continuation<bool> &, const FilePath &, const QByteArray &)> asyncWriteFileContents;
    std::function<bool(const FilePath &, const FilePath &)> ensureReachable;
};

} // namespace Utils

QT_BEGIN_NAMESPACE
QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug dbg, const Utils::FilePath &c);
QT_END_NAMESPACE

Q_DECLARE_METATYPE(Utils::FilePath)

namespace std {
template<>
struct QTCREATOR_UTILS_EXPORT hash<Utils::FilePath>
{
    using argument_type = Utils::FilePath;
    using result_type = size_t;
    result_type operator()(const argument_type &fn) const;
};

} // namespace std
