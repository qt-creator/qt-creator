/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "fileinprojectfinder.h"
#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>
#include <QtCore/QUrl>

namespace Utils {

/*!
  \class Utils::FileInProjectFinder

  \brief Helper class to find the 'original' file in the project directory for a given file url.

  Often files are copied in the build + deploy process. findFile() searches for an existing file
  in the project directory for a given file path:

  E.g. following file paths:
  \list
  \i C:/app-build-desktop/qml/app/main.qml (shadow build directory)
  \i C:/Private/e3026d63/qml/app/main.qml  (Application data folder on Symbian device)
  \i /Users/x/app-build-desktop/App.app/Contents/Resources/qml/App/main.qml (folder on Mac OS X)
  \endlist

 should all be mapped to $PROJECTDIR/qml/app/main.qml
*/

FileInProjectFinder::FileInProjectFinder()
{
}

void FileInProjectFinder::setProjectDirectory(const QString &absoluteProjectPath)
{
    QTC_ASSERT(QFileInfo(absoluteProjectPath).exists()
               && QFileInfo(absoluteProjectPath).isAbsolute(), return);

    if (absoluteProjectPath == m_projectDir)
        return;

    m_projectDir = absoluteProjectPath;
    while (m_projectDir.endsWith(QLatin1Char('/')))
        m_projectDir.remove(m_projectDir.length() - 1, 1);

    m_cache.clear();
}

QString FileInProjectFinder::projectDirectory() const
{
    return m_projectDir;
}

void FileInProjectFinder::setProjectFiles(const QStringList &projectFiles)
{
    m_projectFiles = projectFiles;
    m_cache.clear();
}

/**
  Returns the best match for the given file url in the project directory.

  The method first checks whether the file inside the project directory exists.
  If not, the leading directory in the path is stripped, and the - now shorter - path is
  checked for existence. This continues until either the file is found, or the relative path
  does not contain any directories any more: In this case the path of the url is returned.

  Second, we walk the list of project files, and search for a file name match there.
  */
QString FileInProjectFinder::findFile(const QUrl &fileUrl, bool *success) const
{
    QString originalPath = fileUrl.toLocalFile();
    if (originalPath.isEmpty()) // e.g. qrc://
        originalPath = fileUrl.path();

    if (originalPath.isEmpty()) {
        if (success)
            success = false;
        return originalPath;
    }

    if (!m_projectDir.isEmpty()) {
        int prefixToIgnore = -1;
        const QChar separator = QLatin1Char('/');
        if (originalPath.startsWith(m_projectDir + separator)) {
#ifdef Q_OS_MAC
            // starting with the project path is not sufficient if the file was
            // copied in an insource build, e.g. into MyApp.app/Contents/Resources
            static const QString appResourcePath = QString::fromLatin1(".app/Contents/Resources");
            if (originalPath.contains(appResourcePath)) {
                // the path is inside the project, but most probably as a resource of an insource build
                // so ignore that path
                prefixToIgnore = originalPath.indexOf(appResourcePath) + appResourcePath.length();
            } else {
#endif
                if (success)
                    *success = true;
                return originalPath;
#ifdef Q_OS_MAC
            }
#endif
        }

        if (m_cache.contains(originalPath)) {
            // check if cached path is still there
            QString candidate = m_cache.value(originalPath);
            QFileInfo candidateInfo(candidate);
            if (candidateInfo.exists() && candidateInfo.isFile()) {
                if (success)
                    *success = true;
                return candidate;
            }
        }

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

                m_cache.insert(originalPath, candidate);
                return candidate;
            }
            prefixToIgnore = originalPath.indexOf(separator, prefixToIgnore + 1);
        }
    }

    const QString fileName = QFileInfo(originalPath).fileName();
    foreach (const QString &f, m_projectFiles) {
        if (QFileInfo(f).fileName() == fileName) {
            m_cache.insert(originalPath, f);
            if (success)
                *success = true;
            return f;
        }
    }

    if (success)
        *success = false;

    return originalPath;
}

} // namespace Utils
