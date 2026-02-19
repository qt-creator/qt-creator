// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"

#include <utils/aspects.h>

#include <QReadWriteLock>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT SshSettings : public Utils::AspectContainer
{
public:
    SshSettings();

    Utils::FilePath sshFilePath() const;
    Utils::FilePath sftpFilePath() const;
    Utils::FilePath keygenFilePath() const;
    Utils::FilePath askpassFilePath() const;
    bool useConnectionSharing() const;
    int connectionSharingTimeoutInMinutes() const;

private:
    Utils::FilePathAspect m_sshFilePathAspect{this};
    Utils::FilePathAspect m_sftpFilePathAspect{this};
    Utils::FilePathAspect m_keygenFilePathAspect{this};
    Utils::FilePathAspect m_askpassFilePathAspect{this};
    Utils::BoolAspect m_useConnectionSharingAspect{this};
    Utils::IntegerAspect m_connectionSharingTimeoutInMinutesAspect{this};

    mutable QReadWriteLock m_lock;
};

PROJECTEXPLORER_EXPORT SshSettings &sshSettings();

} // namespace ProjectExplorer
