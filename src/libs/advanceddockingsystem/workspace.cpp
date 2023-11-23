// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "workspace.h"

#include "dockmanager.h"

namespace ADS {

Workspace::Workspace() {}

Workspace::Workspace(const Utils::FilePath &filePath, bool isPreset)
    : m_filePath(filePath)
    , m_preset(isPreset)
{
    if (m_filePath.isEmpty())
        return;

    QString name = DockManager::readDisplayName(m_filePath);

    if (name.isEmpty()) {
        name = baseName();
        // Remove "-" and "_" remove
        name.replace("-", " ");
        name.replace("_", " ");

        setName(name);
    } else {
        m_name = name;
    }
}

void Workspace::setName(const QString &name)
{
    if (DockManager::writeDisplayName(filePath(), name))
        m_name = name;
}

const QString &Workspace::name() const
{
    return m_name;
}

const Utils::FilePath &Workspace::filePath() const
{
    return m_filePath;
}

QString Workspace::fileName() const
{
    return m_filePath.fileName();
}

QString Workspace::baseName() const
{
    return m_filePath.baseName();
}

QDateTime Workspace::lastModified() const
{
    return m_filePath.lastModified();
}

bool Workspace::exists() const
{
    return m_filePath.exists();
}

bool Workspace::isValid() const
{
    if (m_filePath.isEmpty())
        return false;

    return exists();
}

void Workspace::setPreset(bool value)
{
    m_preset = value;
}

bool Workspace::isPreset() const
{
    return m_preset;
}

Workspace::operator QString() const
{
    return QString("Workspace %1 Preset[%2] %3")
        .arg(name())
        .arg(isPreset())
        .arg(filePath().toUserOutput());
}

} // namespace ADS
