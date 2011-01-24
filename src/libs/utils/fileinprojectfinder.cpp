/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "fileinprojectfinder.h"
#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>

namespace Utils {

/**
  \class FileInProjectFinder

  Helper class to find the 'original' file in the project directory for a given file path.

  Often files are copied in the build + deploy process. findFile() searches for an existing file
  in the project directory for a given file path:

  E.g. following file paths:
      C:/app-build-desktop/qml/app/main.qml (shadow build directory)
      C:/Private/e3026d63/qml/app/main.qml  (Application data folder on Symbian device)
      /Users/x/app-build-desktop/App.app/Contents/Resources/qml/App/main.qml (folder on Mac OS X)
 should all be mapped to
      $PROJECTDIR/qml/app/main.qml
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

/**
  Returns the best match for the given originalPath in the project directory.

  The method first checks whether the originalPath inside the project directory exists.
  If not, the leading directory in the path is stripped, and the - now shorter - path is
  checked for existence. This continues until either the file is found, or the relative path
  does not contain any directories any more: In this case the originalPath is returned.
  */
QString FileInProjectFinder::findFile(const QString &originalPath, bool *success) const
{
    if (m_projectDir.isEmpty())
        return originalPath;

    const QChar separator = QLatin1Char('/');
    if (originalPath.startsWith(m_projectDir + separator)) {
        return originalPath;
    }

    if (m_cache.contains(originalPath))
        return m_cache.value(originalPath);

    // Strip directories one by one from the beginning of the path,
    // and see if the new relative path exists in the build directory.
    if (originalPath.contains(separator)) {
        for (int pos = originalPath.indexOf(separator); pos != -1;
             pos = originalPath.indexOf(separator, pos + 1)) {
            QString candidate = originalPath;
            candidate.remove(0, pos);
            candidate.prepend(m_projectDir);
            QFileInfo candidateInfo(candidate);
            if (candidateInfo.exists() && candidateInfo.isFile()) {
                if (success)
                    *success = true;

                m_cache.insert(originalPath, candidate);
                return candidate;
            }
        }
    }

    if (success)
        *success = false;

    return originalPath;
}

} // namespace Utils
