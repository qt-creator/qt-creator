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
#include <QUrl>

#include <algorithm>

enum { debug = false };

namespace Utils {

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

FileInProjectFinder::FileInProjectFinder()
{
}

static QString stripTrailingSlashes(const QString &path)
{
    QString newPath = path;
    while (newPath.endsWith(QLatin1Char('/')))
        newPath.remove(newPath.length() - 1, 1);
    return newPath;
}

void FileInProjectFinder::setProjectDirectory(const QString &absoluteProjectPath)
{
    const QString newProjectPath = stripTrailingSlashes(absoluteProjectPath);

    if (newProjectPath == m_projectDir)
        return;

    const QFileInfo infoPath(newProjectPath);
    QTC_CHECK(newProjectPath.isEmpty()
              || (infoPath.exists() && infoPath.isAbsolute()));

    m_projectDir = newProjectPath;
    m_cache.clear();
}

QString FileInProjectFinder::projectDirectory() const
{
    return m_projectDir;
}

void FileInProjectFinder::setProjectFiles(const QStringList &projectFiles)
{
    if (m_projectFiles == projectFiles)
        return;

    m_projectFiles = projectFiles;
    m_cache.clear();
}

void FileInProjectFinder::setSysroot(const QString &sysroot)
{
    QString newsys = sysroot;
    while (newsys.endsWith(QLatin1Char('/')))
        newsys.remove(newsys.length() - 1, 1);

    if (m_sysroot == newsys)
        return;

    m_sysroot = newsys;
    m_cache.clear();
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
    if (debug)
        qDebug() << "FileInProjectFinder: trying to find file" << fileUrl.toString() << "...";

    QString originalPath = fileUrl.toLocalFile();
    if (originalPath.isEmpty()) // e.g. qrc://
        originalPath = fileUrl.path();

    if (originalPath.isEmpty()) {
        if (debug)
            qDebug() << "FileInProjectFinder: malformed url, returning";
        if (success)
            *success = false;
        return originalPath;
    }

    if (!m_projectDir.isEmpty()) {
        if (debug)
            qDebug() << "FileInProjectFinder: checking project directory ...";

        int prefixToIgnore = -1;
        const QChar separator = QLatin1Char('/');
        if (originalPath.startsWith(m_projectDir + separator)) {
            if (Utils::HostOsInfo::isMacHost()) {
                // starting with the project path is not sufficient if the file was
                // copied in an insource build, e.g. into MyApp.app/Contents/Resources
                static const QString appResourcePath = QString::fromLatin1(".app/Contents/Resources");
                if (originalPath.contains(appResourcePath)) {
                    // the path is inside the project, but most probably as a resource of an insource build
                    // so ignore that path
                    prefixToIgnore = originalPath.indexOf(appResourcePath) + appResourcePath.length();
                }
            }
            if (prefixToIgnore == -1) {
                if (debug)
                    qDebug() << "FileInProjectFinder: found" << originalPath << "in project directory";
                if (success)
                    *success = true;
                return originalPath;
            }
        }

        if (m_cache.contains(originalPath)) {
            if (debug)
                qDebug() << "FileInProjectFinder: checking cache ...";
            // check if cached path is still there
            QString candidate = m_cache.value(originalPath);
            QFileInfo candidateInfo(candidate);
            if (candidateInfo.exists() && candidateInfo.isFile()) {
                if (success)
                    *success = true;
                if (debug)
                    qDebug() << "FileInProjectFinder: found" << candidate << "in the cache";
                return candidate;
            }
        }

        if (debug)
            qDebug() << "FileInProjectFinder: checking stripped paths in project directory ...";

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
            candidate.prepend(m_projectDir);
            QFileInfo candidateInfo(candidate);
            if (candidateInfo.exists() && candidateInfo.isFile()) {
                if (success)
                    *success = true;

                if (debug)
                    qDebug() << "FileInProjectFinder: found" << candidate << "in project directory";

                m_cache.insert(originalPath, candidate);
                return candidate;
            }
            prefixToIgnore = originalPath.indexOf(separator, prefixToIgnore + 1);
        }
    }

    // find best matching file path in project files
    if (debug)
        qDebug() << "FileInProjectFinder: checking project files ...";

    const QString matchedFilePath
            = bestMatch(
                filesWithSameFileName(FileName::fromString(originalPath).fileName()),
                originalPath);
    if (!matchedFilePath.isEmpty()) {
        m_cache.insert(originalPath, matchedFilePath);
        if (success)
            *success = true;
        return matchedFilePath;
    }

    if (findInSearchPaths(&originalPath))
        return originalPath;

    if (debug)
        qDebug() << "FileInProjectFinder: checking absolute path in sysroot ...";

    // check if absolute path is found in sysroot
    if (!m_sysroot.isEmpty()) {
        const QString sysrootPath = m_sysroot + originalPath;
        if (QFileInfo(sysrootPath).exists() && QFileInfo(sysrootPath).isFile()) {
            if (success)
                *success = true;
            m_cache.insert(originalPath, sysrootPath);
            if (debug)
                qDebug() << "FileInProjectFinder: found" << sysrootPath << "in sysroot";
            return sysrootPath;
        }
    }

    if (success)
        *success = false;

    if (debug)
        qDebug() << "FileInProjectFinder: couldn't find file!";
    return originalPath;
}

bool FileInProjectFinder::findInSearchPaths(QString *filePath) const
{
    foreach (const QString &dirPath, m_searchDirectories) {
        if (findInSearchPath(dirPath, filePath))
            return true;
    }
    return false;
}

static void chopFirstDir(QString *dirPath)
{
    int i = dirPath->indexOf(QLatin1Char('/'));
    if (i == -1)
        dirPath->clear();
    else
        dirPath->remove(0, i + 1);
}

bool FileInProjectFinder::findInSearchPath(const QString &searchPath, QString *filePath)
{
    if (debug)
        qDebug() << "FileInProjectFinder: checking search path" << searchPath;

    QFileInfo fi;
    QString s = *filePath;
    while (!s.isEmpty()) {
        fi.setFile(searchPath + QLatin1Char('/') + s);
        if (debug)
            qDebug() << "FileInProjectFinder: trying" << fi.filePath();
        if (fi.exists() && fi.isReadable()) {
            *filePath = fi.filePath();
            return true;
        }
        chopFirstDir(&s);
    }

    return false;
}

QStringList FileInProjectFinder::filesWithSameFileName(const QString &fileName) const
{
    QStringList result;
    foreach (const QString &f, m_projectFiles) {
        if (FileName::fromString(f).fileName() == fileName)
            result << f;
    }
    return result;
}

int FileInProjectFinder::rankFilePath(const QString &candidatePath, const QString &filePathToFind)
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
        if (debug)
            qDebug() << "FileInProjectFinder: found" << filePaths.first() << "in project files";
        return filePaths.first();
    }
    auto it = std::max_element(filePaths.constBegin(), filePaths.constEnd(),
        [&filePathToFind] (const QString &a, const QString &b) -> bool {
            return rankFilePath(a, filePathToFind) < rankFilePath(b, filePathToFind);
    });
    if (it != filePaths.cend()) {
        if (debug)
            qDebug() << "FileInProjectFinder: found best match" << *it << "in project files";
        return *it;
    }
    return QString();
}

QStringList FileInProjectFinder::searchDirectories() const
{
    return m_searchDirectories;
}

void FileInProjectFinder::setAdditionalSearchDirectories(const QStringList &searchDirectories)
{
    m_searchDirectories = searchDirectories;
}


} // namespace Utils
