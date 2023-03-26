// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QSettings>
#include <QSharedPointer>
#include <QString>

namespace Utils {
class FilePath;
} //namespace Utils

namespace McuSupport::Internal {

class SettingsHandler
{
public:
    using Ptr = QSharedPointer<SettingsHandler>;
    virtual ~SettingsHandler() = default;
    virtual Utils::FilePath getPath(const QString &settingsKey,
                                    QSettings::Scope scope,
                                    const Utils::FilePath &m_defaultPath) const;

    virtual bool write(const QString &settingsKey,
                       const Utils::FilePath &path,
                       const Utils::FilePath &defaultPath) const;

    virtual bool isAutomaticKitCreationEnabled() const;
    void setAutomaticKitCreation(bool isEnabled);
}; //class SettingsHandler
} // namespace McuSupport::Internal
