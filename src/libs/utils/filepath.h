// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "expected.h"
#include "filepathinfo.h"
#include "osspecificaspects.h"

#include <QDir>
#include <QDirIterator>
#include <QMetaType>

#include <functional>
#include <memory>
#include <optional>
#include <variant>

QT_BEGIN_NAMESPACE
class QDateTime;
class QDebug;
class QFileInfo;
class QUrl;
QT_END_NAMESPACE

class tst_fileutils; // This becomes a friend of Utils::FilePath for testing private methods.

namespace Utils {

class DeviceFileAccess;
class Environment;
class EnvironmentChange;

template <class ...Args> using Continuation = std::function<void(Args...)>;

class QTCREATOR_UTILS_EXPORT FileFilter
{
public:
    FileFilter(const QStringList &nameFilters,
                   const QDir::Filters fileFilters = QDir::NoFilter,
                   const QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags);

    QStringList asFindArguments(const QString &path) const;

    const QStringList nameFilters;
    const QDir::Filters fileFilters = QDir::NoFilter;
    const QDirIterator::IteratorFlags iteratorFlags = QDirIterator::NoIteratorFlags;
};

using FilePaths = QList<class FilePath>;

enum class IterationPolicy { Stop, Continue };

class QTCREATOR_UTILS_EXPORT FilePath
{
public:
    FilePath();

    template <size_t N> FilePath(const char (&literal)[N]) { setFromString(QString::fromUtf8(literal)); }

    [[nodiscard]] static FilePath fromString(const QString &filepath);
    [[nodiscard]] static FilePath fromStringWithExtension(const QString &filepath, const QString &defaultExtension);
    [[nodiscard]] static FilePath fromUserInput(const QString &filepath);
    [[nodiscard]] static FilePath fromUtf8(const char *filepath, int filepathSize = -1);
    [[nodiscard]] static FilePath fromSettings(const QVariant &variant);
    [[nodiscard]] static FilePath fromVariant(const QVariant &variant);
    [[nodiscard]] static FilePath fromParts(const QStringView scheme, const QStringView host, const QStringView path);
    [[nodiscard]] static FilePath fromPathPart(const QStringView path);

    [[nodiscard]] static FilePath currentWorkingPath();

    QString toUserOutput() const;
    QVariant toSettings() const;
    QVariant toVariant() const;
    QUrl toUrl() const;

    QStringView scheme() const;
    QStringView host() const;
    QStringView pathView() const;
    QString path() const;

    void setParts(const QStringView scheme, const QStringView host, const QStringView path);

    QString fileName() const;
    QStringView fileNameView() const;
    QString fileNameWithPathComponents(int pathComponents) const;

    QString baseName() const;
    QString completeBaseName() const;
    QString suffix() const;
    QStringView suffixView() const;
    QString completeSuffix() const;

    [[nodiscard]] FilePath pathAppended(const QString &str) const;
    [[nodiscard]] FilePath stringAppended(const QString &str) const;
    bool startsWith(const QString &s) const;
    bool endsWith(const QString &s) const;
    bool contains(const QString &s) const;

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
    bool isSymLink() const;
    bool isRootPath() const;
    bool isNewerThan(const QDateTime &timeStamp) const;
    QDateTime lastModified() const;
    QFile::Permissions permissions() const;
    bool setPermissions(QFile::Permissions permissions) const;
    OsType osType() const;
    bool removeFile() const;
    bool removeRecursively(QString *error = nullptr) const;
    expected_str<void> copyRecursively(const FilePath &target) const;
    expected_str<void> copyFile(const FilePath &target) const;
    bool renameFile(const FilePath &target) const;
    qint64 fileSize() const;
    qint64 bytesAvailable() const;
    bool createDir() const;
    FilePaths dirEntries(const FileFilter &filter, QDir::SortFlags sort = QDir::NoSort) const;
    FilePaths dirEntries(QDir::Filters filters) const;
    expected_str<QByteArray> fileContents(qint64 maxSize = -1, qint64 offset = 0) const;
    expected_str<qint64> writeFileContents(const QByteArray &data, qint64 offset = 0) const;
    FilePathInfo filePathInfo() const;

    [[nodiscard]] FilePath operator/(const QString &str) const;

    Qt::CaseSensitivity caseSensitivity() const;
    QChar pathComponentSeparator() const;
    QChar pathListSeparator() const;

    void clear();
    bool isEmpty() const;

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
    [[nodiscard]] FilePath relativePathFrom(const FilePath &anchor) const;
    [[nodiscard]] FilePath searchInDirectories(const FilePaths &dirs) const;
    [[nodiscard]] Environment deviceEnvironment() const;
    [[nodiscard]] FilePath onDevice(const FilePath &deviceTemplate) const;
    [[nodiscard]] FilePath withNewPath(const QString &newPath) const;

    using IterateDirCallback
        = std::variant<
            std::function<IterationPolicy(const FilePath &item)>,
            std::function<IterationPolicy(const FilePath &item, const FilePathInfo &info)>
          >;

    void iterateDirectory(
            const IterateDirCallback &callBack,
            const FileFilter &filter) const;

    static void iterateDirectories(
            const FilePaths &dirs,
            const IterateDirCallback &callBack,
            const FileFilter &filter);

    enum PathAmending { AppendToPath, PrependToPath };
    [[nodiscard]] FilePath searchInPath(const FilePaths &additionalDirs = {},
                                        PathAmending = AppendToPath) const;

    enum MatchScope { ExactMatchOnly, WithExeSuffix, WithBatSuffix,
                      WithExeOrBatSuffix, WithAnySuffix };
    std::optional<FilePath> refersToExecutableFile(MatchScope considerScript) const;

    [[nodiscard]] expected_str<FilePath> tmpDir() const;
    [[nodiscard]] expected_str<FilePath> createTempFile() const;

    // makes sure that capitalization of directories is canonical
    // on Windows and macOS. This is rarely needed.
    [[nodiscard]] FilePath normalizedPathName() const;

    QString displayName(const QString &args = {}) const;
    QString nativePath() const;
    QString shortNativePath() const;
    QString withTildeHomePath() const;

    bool startsWithDriveLetter() const;

    static QString formatFilePaths(const FilePaths &files, const QString &separator);
    static void removeDuplicates(FilePaths &files);
    static void sort(FilePaths &files);

    // Asynchronous interface
    void asyncCopyFile(const Continuation<const expected_str<void> &> &cont,
                       const FilePath &target) const;
    void asyncFileContents(const Continuation<const expected_str<QByteArray> &> &cont,
                           qint64 maxSize = -1,
                           qint64 offset = 0) const;
    void asyncWriteFileContents(const Continuation<const expected_str<qint64> &> &cont,
                                const QByteArray &data,
                                qint64 offset = 0) const;

    // Prefer not to use
    // Using needsDevice() in "user" code is likely to result in code that
    // makes a local/remote distinction which should be avoided in general.
    // There are usually other means available. E.g. distinguishing based
    // on FilePath::osType().
    bool needsDevice() const;
    bool hasFileAccess() const;

    bool isSameDevice(const FilePath &other) const;
    bool isSameFile(const FilePath &other) const;
    bool isSameExecutable(const FilePath &other) const; // with potentially different suffixes

    [[nodiscard]] QFileInfo toFileInfo() const;
    [[nodiscard]] static FilePath fromFileInfo(const QFileInfo &info);

    // Support for FSEngine. Do not use unless needed.
    [[nodiscard]] static const QString &specialRootName();
    [[nodiscard]] static const QString &specialRootPath();
    [[nodiscard]] static const QString &specialDeviceRootName();
    [[nodiscard]] static const QString &specialDeviceRootPath();

    [[nodiscard]] bool ensureReachable(const FilePath &other) const;

    [[nodiscard]] QString toFSPathString() const;

    [[nodiscard]] static int rootLength(const QStringView path); // Assumes no scheme and host
    [[nodiscard]] static int schemeAndHostLength(const QStringView path);

    static QString calcRelativePath(const QString &absolutePath, const QString &absoluteAnchorPath);
    //! Returns a filepath the represents the same file on a local drive
    expected_str<FilePath> localSource() const;

    // FIXME: Avoid. See toSettings, toVariant, toUserOutput, toFSPathString, path, nativePath.
    QString toString() const;

private:
    // These are needed.
    QTCREATOR_UTILS_EXPORT friend bool operator==(const FilePath &first, const FilePath &second);
    QTCREATOR_UTILS_EXPORT friend bool operator!=(const FilePath &first, const FilePath &second);
    QTCREATOR_UTILS_EXPORT friend bool operator<(const FilePath &first, const FilePath &second);
    QTCREATOR_UTILS_EXPORT friend bool operator<=(const FilePath &first, const FilePath &second);
    QTCREATOR_UTILS_EXPORT friend bool operator>(const FilePath &first, const FilePath &second);
    QTCREATOR_UTILS_EXPORT friend bool operator>=(const FilePath &first, const FilePath &second);

    QTCREATOR_UTILS_EXPORT friend size_t qHash(const FilePath &a, uint seed);
    QTCREATOR_UTILS_EXPORT friend size_t qHash(const FilePath &a);

    QTCREATOR_UTILS_EXPORT friend QDebug operator<<(QDebug dbg, const FilePath &c);

    // Implementation details. May change.
    friend class ::tst_fileutils;
    void setPath(QStringView path);
    void setFromString(QStringView filepath);
    DeviceFileAccess *fileAccess() const;

    [[nodiscard]] QString encodedHost() const;

    QString m_data; // Concatenated m_path, m_scheme, m_host
    unsigned int m_pathLen = 0;
    unsigned short m_schemeLen = 0;
    unsigned short m_hostLen = 0;
};

class QTCREATOR_UTILS_EXPORT DeviceFileHooks
{
public:
    static DeviceFileHooks &instance();

    std::function<expected_str<DeviceFileAccess *>(const FilePath &)> fileAccess;
    std::function<QString(const FilePath &)> deviceDisplayName;
    std::function<bool(const FilePath &, const FilePath &)> ensureReachable;
    std::function<Environment(const FilePath &)> environment;
    std::function<bool(const FilePath &left, const FilePath &right)> isSameDevice;
    std::function<expected_str<FilePath>(const FilePath &)> localSource;
    std::function<void(const FilePath &, const Environment &)> openTerminal;
};

} // Utils

Q_DECLARE_METATYPE(Utils::FilePath)

namespace std {

template<> struct hash<Utils::FilePath>
{
    size_t operator()(const Utils::FilePath &fn) const { return qHash(fn); }
};

} // std
