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

    static FilePath fromString(const QString &filepath);
    static FilePath fromFileInfo(const QFileInfo &info);
    static FilePath fromStringWithExtension(const QString &filepath, const QString &defaultExtension);
    static FilePath fromUserInput(const QString &filepath);
    static FilePath fromUtf8(const char *filepath, int filepathSize = -1);
    static FilePath fromVariant(const QVariant &variant);

    QString toString() const;
    FilePath onDevice(const FilePath &deviceTemplate) const;
    FilePath withNewPath(const QString &newPath) const;

    QFileInfo toFileInfo() const;
    QVariant toVariant() const;
    QDir toDir() const;

    QString toUserOutput() const;
    QString shortNativePath() const;

    QString fileName() const;
    QString fileNameWithPathComponents(int pathComponents) const;

    QString baseName() const;
    QString completeBaseName() const;
    QString suffix() const;
    QString completeSuffix() const;

    QString scheme() const { return m_scheme; }
    void setScheme(const QString &scheme);

    QString host() const { return m_host; }
    void setHost(const QString &host);

    QString path() const { return m_data; }
    void setPath(const QString &path) { m_data = path; }

    bool needsDevice() const;
    bool exists() const;

    bool isWritablePath() const { return isWritableDir(); } // Remove.
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

    bool createDir() const;
    QList<FilePath> dirEntries(const QStringList &nameFilters,
                               QDir::Filters filters = QDir::NoFilter,
                               QDir::SortFlags sort = QDir::NoSort) const;
    QList<FilePath> dirEntries(QDir::Filters filters) const;
    QByteArray fileContents(qint64 maxSize = -1, qint64 offset = 0) const;
    bool writeFileContents(const QByteArray &data) const;

    FilePath parentDir() const;
    FilePath absolutePath() const;
    FilePath absoluteFilePath() const;
    FilePath absoluteFilePath(const FilePath &tail) const;

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
    bool startsWithDriveLetter() const;

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

    Qt::CaseSensitivity caseSensitivity() const;

    FilePath relativeChildPath(const FilePath &parent) const;
    FilePath relativePath(const FilePath &anchor) const;
    FilePath pathAppended(const QString &str) const;
    FilePath stringAppended(const QString &str) const;
    FilePath resolvePath(const QString &fileName) const;
    FilePath cleanPath() const;

    FilePath canonicalPath() const;
    FilePath symLinkTarget() const;
    FilePath resolveSymlinks() const;
    FilePath withExecutableSuffix() const;

    FilePath operator/(const QString &str) const;

    void clear();
    bool isEmpty() const;

    uint hash(uint seed) const;

    // NOTE: Most FilePath operations on FilePath created from URL currently
    // do not work. Among the working are .toVariant() and .toUrl().
    static FilePath fromUrl(const QUrl &url);
    QUrl toUrl() const;

    FilePath searchInDirectories(const QList<FilePath> &dirs) const;
    Environment deviceEnvironment() const;

    static QString formatFilePaths(const QList<FilePath> &files, const QString &separator);
    static void removeDuplicates(QList<FilePath> &files);
    static void sort(QList<FilePath> &files);

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
