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

#include <utils/utils_global.h>
#include <utils/fileutils.h>

#include <QHash>
#include <QSharedPointer>
#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QUrl)

namespace Utils {
class QrcParser;

class QTCREATOR_UTILS_EXPORT FileInProjectFinder
{
public:

    using FileHandler = std::function<void(const QString &, int)>;
    using DirectoryHandler = std::function<void(const QStringList &, int)>;

    FileInProjectFinder();
    ~FileInProjectFinder();

    void setProjectDirectory(const FilePath &absoluteProjectPath);
    FilePath projectDirectory() const;

    void setProjectFiles(const FilePaths &projectFiles);
    void setSysroot(const FilePath &sysroot);

    void addMappedPath(const FilePath &localFilePath, const QString &remoteFilePath);

    FilePaths findFile(const QUrl &fileUrl, bool *success = nullptr) const;
    bool findFileOrDirectory(const QString &originalPath, FileHandler fileHandler = nullptr,
                             DirectoryHandler directoryHandler = nullptr) const;

    FilePaths searchDirectories() const;
    void setAdditionalSearchDirectories(const FilePaths &searchDirectories);

private:
    struct PathMappingNode
    {
        ~PathMappingNode();
        FilePath localPath;
        QHash<QString, PathMappingNode *> children;
    };

    struct CacheEntry {
        QStringList paths;
        int matchLength = 0;
    };

    class QrcUrlFinder {
    public:
        FilePaths find(const QUrl &fileUrl) const;
        void setProjectFiles(const FilePaths &projectFiles);
    private:
        FilePaths m_allQrcFiles;
        mutable QHash<QUrl, FilePaths> m_fileCache;
        mutable QHash<FilePath, QSharedPointer<QrcParser>> m_parserCache;
    };

    CacheEntry findInSearchPaths(const QString &filePath, FileHandler fileHandler,
                                 DirectoryHandler directoryHandler) const;
    static CacheEntry findInSearchPath(const QString &searchPath, const QString &filePath,
                                       FileHandler fileHandler, DirectoryHandler directoryHandler);
    QStringList filesWithSameFileName(const QString &fileName) const;
    QStringList pathSegmentsWithSameName(const QString &path) const;

    bool handleSuccess(const QString &originalPath, const QStringList &found, int confidence,
                       const char *where) const;

    static int commonPostFixLength(const QString &candidatePath, const QString &filePathToFind);
    static QStringList bestMatches(const QStringList &filePaths, const QString &filePathToFind);

    FilePath m_projectDir;
    FilePath m_sysroot;
    FilePaths m_projectFiles;
    FilePaths m_searchDirectories;
    PathMappingNode m_pathMapRoot;

    mutable QHash<QString, CacheEntry> m_cache;
    QrcUrlFinder m_qrcUrlFinder;
};

QTCREATOR_UTILS_EXPORT FilePath chooseFileFromList(const FilePaths &candidates);

} // namespace Utils
