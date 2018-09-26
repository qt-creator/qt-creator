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
#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QUrl)

namespace Utils {

class QTCREATOR_UTILS_EXPORT FileInProjectFinder
{
public:

    using FileHandler = std::function<void(const QString &, int)>;
    using DirectoryHandler = std::function<void(const QStringList &, int)>;

    FileInProjectFinder();
    ~FileInProjectFinder();

    void setProjectDirectory(const FileName &absoluteProjectPath);
    FileName projectDirectory() const;

    void setProjectFiles(const FileNameList &projectFiles);
    void setSysroot(const FileName &sysroot);

    void addMappedPath(const FileName &localFilePath, const QString &remoteFilePath);

    QString findFile(const QUrl &fileUrl, bool *success = nullptr) const;
    bool findFileOrDirectory(const QString &originalPath, FileHandler fileHandler = nullptr,
                             DirectoryHandler directoryHandler = nullptr) const;

    FileNameList searchDirectories() const;
    void setAdditionalSearchDirectories(const FileNameList &searchDirectories);

private:
    struct PathMappingNode
    {
        ~PathMappingNode();
        FileName localPath;
        QHash<QString, PathMappingNode *> children;
    };

    struct CacheEntry {
        QString path;
        int matchLength = 0;
    };

    CacheEntry findInSearchPaths(const QString &filePath, FileHandler fileHandler,
                                 DirectoryHandler directoryHandler) const;
    static CacheEntry findInSearchPath(const QString &searchPath, const QString &filePath,
                                       FileHandler fileHandler, DirectoryHandler directoryHandler);
    QStringList filesWithSameFileName(const QString &fileName) const;
    QStringList pathSegmentsWithSameName(const QString &path) const;

    bool handleSuccess(const QString &originalPath, const QString &found, int confidence,
                       const char *where) const;

    static int commonPostFixLength(const QString &candidatePath, const QString &filePathToFind);
    static QString bestMatch(const QStringList &filePaths, const QString &filePathToFind);

    FileName m_projectDir;
    FileName m_sysroot;
    FileNameList m_projectFiles;
    FileNameList m_searchDirectories;
    PathMappingNode m_pathMapRoot;

    mutable QHash<QString, CacheEntry> m_cache;
};

} // namespace Utils
