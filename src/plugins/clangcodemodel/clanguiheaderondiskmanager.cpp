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

#include "clanguiheaderondiskmanager.h"

#include <QFile>
#include <QFileInfo>

#include <utils/qtcassert.h>

namespace ClangCodeModel {
namespace Internal {

UiHeaderOnDiskManager::UiHeaderOnDiskManager() : m_temporaryDir("clang-uiheader-XXXXXX")
{
    QTC_CHECK(m_temporaryDir.isValid());
}

QString UiHeaderOnDiskManager::createIfNeeded(const QString &filePath)
{
    const QString mappedPath = mapPath(filePath);
    if (!QFileInfo::exists(mappedPath)) {
        const bool fileCreated = QFile(mappedPath).open(QFile::WriteOnly); // touch file
        QTC_CHECK(fileCreated);
    }

    return mappedPath;
}

QString UiHeaderOnDiskManager::remove(const QString &filePath)
{
    const QString mappedPath = mapPath(filePath);
    if (QFileInfo::exists(mappedPath)) {
        const bool fileRemoved = QFile::remove(mappedPath);
        QTC_CHECK(fileRemoved);
    }

    return mappedPath;
}

QString UiHeaderOnDiskManager::directoryPath() const
{
    return m_temporaryDir.path();
}

QString UiHeaderOnDiskManager::mapPath(const QString &filePath) const
{
    return directoryPath() + '/' + QFileInfo(filePath).fileName();
}

} // namespace Internal
} // namespace ClangCodeModel
