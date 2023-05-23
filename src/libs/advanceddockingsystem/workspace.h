// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "ads_globals.h"

#include <utils/fileutils.h>

namespace ADS {

class ADS_EXPORT Workspace
{
public:
    Workspace();
    Workspace(const Utils::FilePath &filePath, bool isPreset = false);

    void setName(const QString &name);
    const QString &name() const;

    const Utils::FilePath &filePath() const;

    QString fileName() const;
    QString baseName() const;
    QDateTime lastModified() const;
    bool exists() const;

    bool isValid() const;

    void setPreset(bool value);
    bool isPreset() const;

    friend bool operator==(const Workspace &a, const Workspace &b)
    {
        return a.fileName() == b.fileName();
    }

    friend bool operator==(const QString &fileName, const Workspace &workspace)
    {
        return fileName == workspace.fileName();
    }
    friend bool operator==(const Workspace &workspace, const QString &fileName)
    {
        return workspace.fileName() == fileName;
    }

    explicit operator QString() const;

private:
    QString m_name;
    Utils::FilePath m_filePath;
    bool m_preset = false;
};

} // namespace ADS
