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

#include "qmlprojectitem.h"
#include "filefilteritems.h"

#include <utils/algorithm.h>

#include <QDir>

namespace QmlProjectManager {

// kind of initialization
void QmlProjectItem::setSourceDirectory(const QString &directoryPath)
{
    if (m_sourceDirectory == directoryPath)
        return;

    m_sourceDirectory = directoryPath;

    for (auto contentElement : qAsConst(m_content)) {
        auto fileFilter = qobject_cast<FileFilterBaseItem*>(contentElement);
        if (fileFilter) {
            fileFilter->setDefaultDirectory(directoryPath);
            connect(fileFilter, &FileFilterBaseItem::filesChanged,
                    this, &QmlProjectItem::qmlFilesChanged);
        }
    }
}

void QmlProjectItem::setTargetDirectory(const QString &directoryPath)
{
    m_targetDirectory = directoryPath;
}

void QmlProjectItem::setQtForMCUs(bool b)
{
    m_qtForMCUs = b;
}

void QmlProjectItem::setImportPaths(const QStringList &importPaths)
{
    if (m_importPaths != importPaths)
        m_importPaths = importPaths;
}

void QmlProjectItem::setFileSelectors(const QStringList &selectors)
{
    if (m_fileSelectors != selectors)
        m_fileSelectors = selectors;
}

/* Returns list of absolute paths */
QStringList QmlProjectItem::files() const
{
    QSet<QString> files;

    for (const auto contentElement : qAsConst(m_content)) {
        if (auto fileFilter = qobject_cast<const FileFilterBaseItem *>(contentElement)) {
            const QStringList fileList = fileFilter->files();
            for (const QString &file : fileList) {
                files.insert(file);
            }
        }
    }
    return Utils::toList(files);
}

/**
  Check whether the project would include a file path
  - regardless whether the file already exists or not.

  @param filePath: absolute file path to check
  */
bool QmlProjectItem::matchesFile(const QString &filePath) const
{
    return Utils::contains(m_content, [&filePath](const QmlProjectContentItem *item) {
        auto fileFilter = qobject_cast<const FileFilterBaseItem *>(item);
        return fileFilter && fileFilter->matchesFile(filePath);
    });
}

void QmlProjectItem::setForceFreeType(bool b)
{
    m_forceFreeType = b;
}

Utils::EnvironmentItems QmlProjectItem::environment() const
{
    return m_environment;
}

void QmlProjectItem::addToEnviroment(const QString &key, const QString &value)
{
    m_environment.append(Utils::EnvironmentItem(key, value));
}

} // namespace QmlProjectManager
