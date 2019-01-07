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

#include "fileinprojectfinder.h"
#include "fileutils.h"
#include "hostosinfo.h"
#include "qtcassert.h"

#include <QDebug>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QUrl>
#include <QDir>

#include <algorithm>

namespace {
Q_LOGGING_CATEGORY(finderLog, "qtc.utils.fileinprojectfinder", QtWarningMsg);
}

namespace Utils {

static bool checkPath(const QString &candidate, int matchLength,
                      FileInProjectFinder::FileHandler fileHandler,
                      FileInProjectFinder::DirectoryHandler directoryHandler)
{
    const QFileInfo candidateInfo(candidate);
    if (fileHandler && candidateInfo.isFile()) {
        fileHandler(candidate, matchLength);
        return true;
    } else if (directoryHandler && candidateInfo.isDir()) {
        directoryHandler(QDir(candidate).entryList(), matchLength);
        return true;
    }
    return false;
}

/*!
  \class Utils::FileInProjectFinder

  \brief The FileInProjectFinder class is a helper class to find the \e original
  file in the project directory for a given file URL.

  Often files are copied in the build and deploy process. findFile() searches for an existing file
  in the project directory for a given file path.

  For example, the following file paths should all be mapped to
  $PROJECTDIR/qml/app/main.qml:
  \list
  \li C:/app-build-desktop/qml/app/main.qml (shadow build directory)
  \li /Users/x/app-build-desktop/App.app/Contents/Resources/qml/App/main.qml (folder on Mac OS X)
  \endlist
*/

FileInProjectFinder::FileInProjectFinder() = default;
FileInProjectFinder::~FileInProjectFinder() = default;

void FileInProjectFinder::setProjectDirectory(const FileName &absoluteProjectPath)
{
    if (absoluteProjectPath == m_projectDir)
        return;

    const QFileInfo infoPath = absoluteProjectPath.toFileInfo();
    QTC_CHECK(absoluteProjectPath.isEmpty()
              || (infoPath.exists() && infoPath.isAbsolute()));

    m_projectDir = absoluteProjectPath;
    m_cache.clear();
}

FileName  FileInProjectFinder::projectDirectory() const
{
    return m_projectDir;
}

void FileInProjectFinder::setProjectFiles(const FileNameList &projectFiles)
{
    if (m_projectFiles == projectFiles)
        return;

    m_projectFiles = projectFiles;
    m_cache.clear();
}

void FileInProjectFinder::setSysroot(const FileName &sysroot)
{
    if (m_sysroot == sysroot)
        return;

    m_sysroot = sysroot;
    m_cache.clear();
}

void FileInProjectFinder::addMappedPath(const FileName &localFilePath, const QString &remoteFilePath)
{
    const QStringList segments = remoteFilePath.split('/', QString::SkipEmptyParts);

    PathMappingNode *node = &m_pathMapRoot;
    for (const QString &segment : segments) {
        auto it = node->children.find(segment);
        if (it == node->children.end())
            it = node->children.insert(segment, new PathMappingNode);
        node = *it;
    }
    node->localPath = localFilePath;
}

/*!
  Returns the best match for the given file URL in the project directory.

  The function first checks whether the file inside the project directory exists.
  If not, the leading directory in the path is stripped, and the - now shorter - path is
  checked for existence, and so on. Second, it tries to locate the file in the sysroot
  folder specified. Third, we walk the list of project files, and search for a file name match
  there. If all fails, it returns the original path from the file URL.
  */
QString FileInProjectFinder::findFile(const QUrl &fileUrl, bool *success) const
{
    qCDebug(finderLog) << "FileInProjectFinder: trying to find file" << fileUrl.toString() << "...";

    QString originalPath = fileUrl.toLocalFile();
    if (originalPath.isEmpty()) // e.g. qrc://
        originalPath = fileUrl.path();

    QString result = originalPath;
    bool found = findFileOrDirectory(originalPath, [&](const QString &fileName, int) {
        result = fileName;
    });

    if (success)
        *success = found;

    return result;
}

bool FileInProjectFinder::handleSuccess(const QString &originalPath, const QString &found,
                                        int matchLength, const char *where) const
{
    qCDebug(finderLog) << "FileInProjectFinder: found" << found << where;
    CacheEntry entry;
    entry.path = found;
    entry.matchLength = matchLength;
    m_cache.insert(originalPath, entry);
    return true;
}

bool FileInProjectFinder::findFileOrDirectory(const QString &originalPath, FileHandler fileHandler,
                                              DirectoryHandler directoryHandler) const
{
    if (originalPath.isEmpty()) {
        qCDebug(finderLog) << "FileInProjectFinder: malformed original path, returning";
        return false;
    }

    const auto segments = originalPath.splitRef('/', QString::SkipEmptyParts);
    const PathMappingNode *node = &m_pathMapRoot;
    for (const auto &segment : segments) {
        auto it = node->children.find(segment.toString());
        if (it == node->children.end()) {
            node = nullptr;
            break;
        }
        node = *it;
    }

    const int origLength = originalPath.length();
    if (node) {
        if (!node->localPath.isEmpty()) {
            const QString localPath = node->localPath.toString();
            if (checkPath(localPath, origLength, fileHandler, directoryHandler))
                return handleSuccess(originalPath, localPath, origLength, "in mapped paths");
        } else if (directoryHandler) {
            directoryHandler(node->children.keys(), origLength);
            qCDebug(finderLog) << "FileInProjectFinder: found virtual directory" << originalPath
                               << "in mapped paths";
            return true;
        }
    }

    auto it = m_cache.find(originalPath);
    if (it != m_cache.end()) {
        qCDebug(finderLog) << "FileInProjectFinder: checking cache ...";
        // check if cached path is still there
        const CacheEntry &candidate = it.value();
        if (checkPath(candidate.path, candidate.matchLength, fileHandler, directoryHandler)) {
            qCDebug(finderLog) << "FileInProjectFinder: found" << candidate.path << "in the cache";
            return true;
        } else {
            m_cache.erase(it);
        }
    }

    if (!m_projectDir.isEmpty()) {
        qCDebug(finderLog) << "FileInProjectFinder: checking project directory ...";

        int prefixToIgnore = -1;
        const QChar separator = QLatin1Char('/');
        if (originalPath.startsWith(m_projectDir.toString() + separator)) {
            if (HostOsInfo::isMacHost()) {
                // starting with the project path is not sufficient if the file was
                // copied in an insource build, e.g. into MyApp.app/Contents/Resources
                static const QString appResourcePath = QString::fromLatin1(".app/Contents/Resources");
                if (originalPath.contains(appResourcePath)) {
                    // the path is inside the project, but most probably as a resource of an insource build
                    // so ignore that path
                    prefixToIgnore = originalPath.indexOf(appResourcePath) + appResourcePath.length();
                }
            }

            if (prefixToIgnore == -1
                    && checkPath(originalPath, origLength, fileHandler, directoryHandler)) {
                return handleSuccess(originalPath, originalPath, origLength,
                                     "in project directory");
            }
        }

        qCDebug(finderLog) << "FileInProjectFinder:"
                           << "checking stripped paths in project directory ...";

        // Strip directories one by one from the beginning of the path,
        // and see if the new relative path exists in the build directory.
        if (prefixToIgnore < 0) {
            if (!QFileInfo(originalPath).isAbsolute()
                    && !originalPath.startsWith(separator)) {
                prefixToIgnore = 0;
            } else {
                prefixToIgnore = originalPath.indexOf(separator);
            }
        }
        while (prefixToIgnore != -1) {
            QString candidate = originalPath;
            candidate.remove(0, prefixToIgnore);
            candidate.prepend(m_projectDir.toString());
            const int matchLength = origLength - prefixToIgnore;
            if (checkPath(candidate, matchLength, fileHandler, directoryHandler))
                return handleSuccess(originalPath, candidate, matchLength, "in project directory");
            prefixToIgnore = originalPath.indexOf(separator, prefixToIgnore + 1);
        }
    }

    // find best matching file path in project files
    qCDebug(finderLog) << "FileInProjectFinder: checking project files ...";

    QStringList matches;
    const QString lastSegment = FileName::fromString(originalPath).fileName();
    if (fileHandler)
        matches.append(filesWithSameFileName(lastSegment));
    if (directoryHandler)
        matches.append(pathSegmentsWithSameName(lastSegment));
    const QString matchedFilePath = bestMatch(matches, originalPath);
    const int matchLength = commonPostFixLength(matchedFilePath, originalPath);
    if (!matchedFilePath.isEmpty()
            && checkPath(matchedFilePath, matchLength, fileHandler, directoryHandler)) {
        return handleSuccess(originalPath, matchedFilePath, matchLength,
                             "when matching project files");
    }

    CacheEntry foundPath = findInSearchPaths(originalPath, fileHandler, directoryHandler);
    if (!foundPath.path.isEmpty())
        return handleSuccess(originalPath, foundPath.path, foundPath.matchLength, "in search path");

    qCDebug(finderLog) << "FileInProjectFinder: checking absolute path in sysroot ...";

    // check if absolute path is found in sysroot
    if (!m_sysroot.isEmpty()) {
        FileName sysrootPath = m_sysroot;
        sysrootPath.appendPath(originalPath);
        if (checkPath(sysrootPath.toString(), origLength, fileHandler, directoryHandler))
            return handleSuccess(originalPath, sysrootPath.toString(), origLength, "in sysroot");
    }

    qCDebug(finderLog) << "FileInProjectFinder: couldn't find file!";

    return false;
}

FileInProjectFinder::CacheEntry FileInProjectFinder::findInSearchPaths(
        const QString &filePath, FileHandler fileHandler, DirectoryHandler directoryHandler) const
{
    for (const FileName &dirPath : m_searchDirectories) {
        const CacheEntry found = findInSearchPath(dirPath.toString(), filePath,
                                                  fileHandler, directoryHandler);
        if (!found.path.isEmpty())
            return found;
    }

    return CacheEntry();
}

static QString chopFirstDir(const QString &dirPath)
{
    int i = dirPath.indexOf(QLatin1Char('/'));
    if (i == -1)
        return QString();
    else
        return dirPath.mid(i + 1);
}

FileInProjectFinder::CacheEntry FileInProjectFinder::findInSearchPath(
        const QString &searchPath, const QString &filePath,
        FileHandler fileHandler, DirectoryHandler directoryHandler)
{
    qCDebug(finderLog) << "FileInProjectFinder: checking search path" << searchPath;

    QString s = filePath;
    while (!s.isEmpty()) {
        CacheEntry result;
        result.path = searchPath + QLatin1Char('/') + s;
        result.matchLength = s.length() + 1;
        qCDebug(finderLog) << "FileInProjectFinder: trying" << result.path;

        if (checkPath(result.path, result.matchLength, fileHandler, directoryHandler))
            return result;

        QString next = chopFirstDir(s);
        if (next.isEmpty()) {
            if (directoryHandler && QFileInfo(searchPath).fileName() == s) {
                result.path = searchPath;
                directoryHandler(QDir(searchPath).entryList(), result.matchLength);
                return result;
            }
            break;
        }
        s = next;
    }

    return CacheEntry();
}

QStringList FileInProjectFinder::filesWithSameFileName(const QString &fileName) const
{
    QStringList result;
    foreach (const FileName &f, m_projectFiles) {
        if (f.fileName() == fileName)
            result << f.toString();
    }
    return result;
}

QStringList FileInProjectFinder::pathSegmentsWithSameName(const QString &pathSegment) const
{
    QStringList result;
    for (const FileName &f : m_projectFiles) {
        FileName currentPath = f.parentDir();
        do {
            if (currentPath.fileName() == pathSegment) {
                if (result.isEmpty() || result.last() != currentPath.toString())
                    result.append(currentPath.toString());
            }
            currentPath = currentPath.parentDir();
        } while (!currentPath.isEmpty());
    }
    result.removeDuplicates();
    return result;
}

int FileInProjectFinder::commonPostFixLength(const QString &candidatePath,
                                             const QString &filePathToFind)
{
    int rank = 0;
    for (int a = candidatePath.length(), b = filePathToFind.length();
         --a >= 0 && --b >= 0 && candidatePath.at(a) == filePathToFind.at(b);)
        rank++;
    return rank;
}

QString FileInProjectFinder::bestMatch(const QStringList &filePaths, const QString &filePathToFind)
{
    if (filePaths.isEmpty())
        return QString();
    if (filePaths.length() == 1) {
        qCDebug(finderLog) << "FileInProjectFinder: found" << filePaths.first()
                           << "in project files";
        return filePaths.first();
    }
    auto it = std::max_element(filePaths.constBegin(), filePaths.constEnd(),
        [&filePathToFind] (const QString &a, const QString &b) -> bool {
            return commonPostFixLength(a, filePathToFind) < commonPostFixLength(b, filePathToFind);
    });
    if (it != filePaths.cend()) {
        qCDebug(finderLog) << "FileInProjectFinder: found best match" << *it << "in project files";
        return *it;
    }
    return QString();
}

FileNameList FileInProjectFinder::searchDirectories() const
{
    return m_searchDirectories;
}

void FileInProjectFinder::setAdditionalSearchDirectories(const FileNameList &searchDirectories)
{
    m_searchDirectories = searchDirectories;
}

FileInProjectFinder::PathMappingNode::~PathMappingNode()
{
    qDeleteAll(children);
}

} // namespace Utils
