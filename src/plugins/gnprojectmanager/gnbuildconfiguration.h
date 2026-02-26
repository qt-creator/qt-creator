// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildconfiguration.h>

namespace GNProjectManager::Internal {

class GNBuildConfiguration final : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
public:
    GNBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);

    void build(const QString &target);

    Utils::FilePath gnExecutable() const;
    QStringList gnGenArgs() const;

    const QString &parameters() const;
    void setParameters(const QString &params);

signals:
    void parametersChanged();

private:
    void toMap(Utils::Store &map) const override;
    void fromMap(const Utils::Store &map) override;

    QWidget *createConfigWidget() final;
    QString m_parameters;
};

void setupGNBuildConfiguration();

} // namespace GNProjectManager::Internal
