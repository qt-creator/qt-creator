// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "deployablefile.h"

#include <utils/fileutils.h>

#include <QHash>

using namespace Utils;

namespace ProjectExplorer {

DeployableFile::DeployableFile(const FilePath &localFilePath, const QString &remoteDir, Type type)
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

size_t qHash(const DeployableFile &d)
{
    return qHash(qMakePair(d.localFilePath().toString(), d.remoteDirectory()));
}

} // namespace ProjectExplorer
