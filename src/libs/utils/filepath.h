/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QDir>
#include <QDirIterator>
#include <QMetaType>

#include <functional>
#include <memory>

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

    QString toUserOutput() const;
    QString toString() const;
    QVariant toVariant() const;
    QUrl toUrl() const;

    QString scheme() const { return m_scheme; }
    void setScheme(const QString &scheme);

    QString host() const { return m_host; }
    void setHost(const QString &host);

    QString path() const { return m_data; }
    void setPath(const QString &path) { m_data = path; }

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
    bool needsDevice() const;

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
    bool createDir() const;
    QList<FilePath> dirEntries(const QStringList &nameFilters,
                               QDir::Filters filters = QDir::NoFilter,
                               QDir::SortFlags sort = QDir::NoSort) const;
    QList<FilePath> dirEntries(QDir::Filters filters) const;
    QByteArray fileContents(qint64 maxSize = -1, qint64 offset = 0) const;
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

    uint hash(uint seed) const;

    [[nodiscard]] FilePath resolvePath(const FilePath &tail) const;
    [[nodiscard]] FilePath resolvePath(const QString &tail) const;
    [[nodiscard]] FilePath cleanPath() const;
    [[nodiscard]] FilePath canonicalPath() const;
    [[nodiscard]] FilePath symLinkTarget() const;
    [[nodiscard]] FilePath resolveSymlinks() const;
    [[nodiscard]] FilePath withExecutableSuffix() const;
    [[nodiscard]] FilePath relativeChildPath(const FilePath &parent) const;
    [[nodiscard]] FilePath relativePath(const FilePath &anchor) const;
    [[nodiscard]] FilePath searchInDirectories(const QList<FilePath> &dirs) const;
    [[nodiscard]] FilePath searchInPath(const QList<FilePath> &additionalDirs = {}) const;
    [[nodiscard]] Environment deviceEnvironment() const;
    [[nodiscard]] FilePath onDevice(const FilePath &deviceTemplate) const;
    [[nodiscard]] FilePath withNewPath(const QString &newPath) const;
    void iterateDirectory(const std::function<bool(const FilePath &item)> &callBack,
                          const QStringList &nameFilters,
                          QDir::Filters filters = QDir::NoFilter,
                          QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags) const;

    // makes sure that capitalization of directories is canonical
    // on Windows and macOS. This is rarely needed.
    [[nodiscard]] FilePath normalizedPathName() const;

    QString shortNativePath() const;
    bool startsWithDriveLetter() const;

    static QString formatFilePaths(const QList<FilePath> &files, const QString &separator);
    static void removeDuplicates(QList<FilePath> &files);
    static void sort(QList<FilePath> &files);

    // Asynchronous interface
    template <class ...Args> using Continuation = std::function<void(Args...)>;
    void asyncCopyFile(const Continuation<bool> &cont, const FilePath &target) const;
    void asyncFileContents(const Continuation<const QByteArray &> &cont,
                           qint64 maxSize = -1, qint64 offset = 0) const;
    void asyncWriteFileContents(const Continuation<bool> &cont, const QByteArray &data) const;

    // Deprecated.
    [[nodiscard]] static FilePath fromFileInfo(const QFileInfo &info); // Avoid.
    [[nodiscard]] QFileInfo toFileInfo() const; // Avoid.
    [[nodiscard]] QDir toDir() const; // Avoid.
    [[nodiscard]] FilePath absolutePath() const; // Avoid. Use resolvePath(...)[.parent()] with proper base.
    [[nodiscard]] FilePath absoluteFilePath() const; // Avoid. Use resolvePath(...) with proper base.

private:
    friend class ::tst_fileutils;
    static QString calcRelativePath(const QString &absolutePath, const QString &absoluteAnchorPath);
    void setFromString(const QString &filepath);

    QString m_scheme;
    QString m_host;
    QString m_data;
};

using FilePaths = QList<FilePath>;

} // namespace Utils

QT_BEGIN_NAMESPACE
QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug dbg, const Utils::FilePath &c);
QT_END_NAMESPACE

Q_DECLARE_METATYPE(Utils::FilePath)
