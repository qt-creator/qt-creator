// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/filepath.h>

#include <QString>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT DeployableFile
{
public:
    enum Type
    {
        TypeNormal,
        TypeExecutable
    };

    DeployableFile() = default;
    DeployableFile(const Utils::FilePath &localFilePath, const QString &remoteDir,
                   Type type = TypeNormal);

    Utils::FilePath localFilePath() const { return m_localFilePath; }
    QString remoteDirectory() const { return m_remoteDir; }
    QString remoteFilePath() const;

    bool isValid() const;

    bool isExecutable() const;

    friend bool operator==(const DeployableFile &d1, const DeployableFile &d2)
    {
        return d1.localFilePath() == d2.localFilePath() && d1.remoteDirectory() == d2.remoteDirectory();
    }

    friend bool operator!=(const DeployableFile &d1, const DeployableFile &d2)
    {
        return !(d1 == d2);
    }
    friend PROJECTEXPLORER_EXPORT size_t qHash(const DeployableFile &d);

private:
    Utils::FilePath m_localFilePath;
    QString m_remoteDir;
    Type m_type = TypeNormal;
};

} // namespace ProjectExplorer
