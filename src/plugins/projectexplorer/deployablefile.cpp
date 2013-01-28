/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "deployablefile.h"

#include <QFileInfo>
#include <QHash>
#include <QPair>

using namespace Utils;

namespace ProjectExplorer {

DeployableFile::DeployableFile()
    : m_type(TypeNormal)
{
}

DeployableFile::DeployableFile(const QString &localFilePath, const QString &remoteDir, Type type)
    : m_localFilePath(FileName::fromUserInput(localFilePath)), m_remoteDir(remoteDir), m_type(type)
{
}

DeployableFile::DeployableFile(const FileName &localFilePath, const QString &remoteDir, Type type)
    : m_localFilePath(localFilePath), m_remoteDir(remoteDir), m_type(type)
{
}

QString DeployableFile::remoteFilePath() const
{
    return m_remoteDir.isEmpty()
            ? QString() : m_remoteDir + QLatin1Char('/') + m_localFilePath.toFileInfo().fileName();
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
