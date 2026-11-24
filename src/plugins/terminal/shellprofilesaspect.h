// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "shellmodel.h"

#include <utils/aspects.h>

#include <QStringList>

#include <functional>

namespace Terminal::Internal {

class ShellProfileSettings
{
public:
    friend bool operator==(const ShellProfileSettings &, const ShellProfileSettings &) = default;

    QList<ShellProfile> manualProfiles;
    QStringList hiddenProfiles;
    QStringList detectedOrder;
    QString defaultProfile;
};

QList<ShellProfile> orderedShellProfiles(const ShellProfileSettings &settings,
                                         const QList<ShellProfile> &detected);
QList<ShellProfile> visibleShellProfiles(const ShellProfileSettings &settings,
                                         const QList<ShellProfile> &detected);
ShellProfile defaultShellProfile(const ShellProfileSettings &settings,
                                 const ShellProfile &system,
                                 const std::function<QList<ShellProfile>()> &detected);

QVariant shellProfileSettingsToVariant(const ShellProfileSettings &settings);
ShellProfileSettings shellProfileSettingsFromVariant(const QVariant &variant);

class ShellProfilesAspect final : public Utils::TypedAspect<ShellProfileSettings>
{
public:
    explicit ShellProfilesAspect(Utils::AspectContainer *container);

    QList<ShellProfile> profiles() const;
    ShellProfile defaultProfile() const;

    void addToLayoutImpl(Layouting::Layout &parent) override;

    void writeSettings() const override;

    QVariant toSettingsValue(const QVariant &valueToSave) const override;
    QVariant fromSettingsValue(const QVariant &savedValue) const override;
};

void migrateLegacyShellSetting(ShellProfilesAspect &shellProfiles, const Utils::Key &group);

} // namespace Terminal::Internal
