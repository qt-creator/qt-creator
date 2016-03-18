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

#include <QHash>
#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QUrl)

namespace Utils {

class QTCREATOR_UTILS_EXPORT FileInProjectFinder
{
public:
    FileInProjectFinder();

    void setProjectDirectory(const QString &absoluteProjectPath);
    QString projectDirectory() const;

    void setProjectFiles(const QStringList &projectFiles);
    void setSysroot(const QString &sysroot);

    QString findFile(const QUrl &fileUrl, bool *success = 0) const;

    QStringList searchDirectories() const;
    void setAdditionalSearchDirectories(const QStringList &searchDirectories);

private:
    bool findInSearchPaths(QString *filePath) const;
    static bool findInSearchPath(const QString &searchPath, QString *filePath);
    QStringList filesWithSameFileName(const QString &fileName) const;
    static int rankFilePath(const QString &candidatePath, const QString &filePathToFind);
    static QString bestMatch(const QStringList &filePaths, const QString &filePathToFind);

    QString m_projectDir;
    QString m_sysroot;
    QStringList m_projectFiles;
    QStringList m_searchDirectories;
    mutable QHash<QString,QString> m_cache;
};

} // namespace Utils
