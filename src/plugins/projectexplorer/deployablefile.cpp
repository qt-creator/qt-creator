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

#include "deployablefile.h"

#include <utils/fileutils.h>

#include <QHash>

using namespace Utils;

namespace ProjectExplorer {

DeployableFile::DeployableFile(const QString &localFilePath, const QString &remoteDir, Type type)
    : m_localFilePath(FileName::fromUserInput(localFilePath)), m_remoteDir(remoteDir), m_type(type)
{ }

DeployableFile::DeployableFile(const FileName &localFilePath, const QString &remoteDir, Type type)
    : m_localFilePath(localFilePath), m_remoteDir(remoteDir), m_type(type)
{ }

QString DeployableFile::remoteFilePath() const
{
    return m_remoteDir.isEmpty()
            ? QString() : m_remoteDir + QLatin1Char('/') + m_localFilePath.fileName();
}

bool DeployableFile::isValid() const
{
    return !m_localFilePath.toString().isEmpty() && !m_remoteDir.isEmpty();
}

bool DeployableFile::isExecutable() const
{
    return m_type == TypeExecutable;
}

uint qHash(const DeployableFile &d)
{
    return qHash(qMakePair(d.localFilePath().toString(), d.remoteDirectory()));
}

} // namespace ProjectExplorer
