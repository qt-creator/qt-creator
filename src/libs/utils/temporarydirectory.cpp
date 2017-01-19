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

#include "temporarydirectory.h"

#include "qtcassert.h"

#include <memory>

namespace Utils {

static std::unique_ptr<QTemporaryDir> m_masterTemporaryDir;

TemporaryDirectory::TemporaryDirectory(const QString &pattern) :
    QTemporaryDir(m_masterTemporaryDir->path() + '/' + pattern)
{
    QTC_CHECK(!QFileInfo(pattern).isAbsolute());
}

QTemporaryDir *TemporaryDirectory::masterTemporaryDirectory()
{
    return m_masterTemporaryDir.get();
}

void TemporaryDirectory::setMasterTemporaryDirectory(const QString &pattern)
{
    m_masterTemporaryDir = std::make_unique<QTemporaryDir>(pattern);
}

QString TemporaryDirectory::masterDirectoryPath()
{
    return m_masterTemporaryDir->path();
}

} // namespace Utils
