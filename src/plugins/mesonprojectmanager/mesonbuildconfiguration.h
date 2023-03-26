// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "projectexplorer/buildconfiguration.h"
#include "projectexplorer/target.h"

namespace MesonProjectManager {
namespace Internal {

enum class MesonBuildType { plain, debug, debugoptimized, release, minsize, custom };

const QHash<QString, MesonBuildType> buildTypesByName = {{"plain", MesonBuildType::plain},
                                                         {"debug", MesonBuildType::debug},
                                                         {"debugoptimized",
                                                          MesonBuildType::debugoptimized},
                                                         {"release", MesonBuildType::release},
                                                         {"minsize", MesonBuildType::minsize},
                                                         {"custom", MesonBuildType::custom}};

inline QString mesonBuildTypeName(MesonBuildType type)
{
    return buildTypesByName.key(type, "custom");
}

inline QString mesonBuildTypeDisplayName(MesonBuildType type)
{
    switch (type) {
    case MesonBuildType::plain:
        return {"Plain"};
    case MesonBuildType::debug:
        return {"Debug"};
    case MesonBuildType::debugoptimized:
        return {"Debug With Optimizations"};
    case MesonBuildType::release:
        return {"Release"};
    case MesonBuildType::minsize:
        return {"Minimum Size"};
    default:
        return {"Custom"};
    }
}

inline MesonBuildType mesonBuildType(const QString &typeName)
{
    return buildTypesByName.value(typeName, MesonBuildType::custom);
}

inline ProjectExplorer::BuildConfiguration::BuildType buildType(MesonBuildType type)
{
    switch (type) {
    case MesonBuildType::plain:
        return ProjectExplorer::BuildConfiguration::Unknown;
    case MesonBuildType::debug:
        return ProjectExplorer::BuildConfiguration::Debug;
    case MesonBuildType::debugoptimized:
        return ProjectExplorer::BuildConfiguration::Profile;
    case MesonBuildType::release:
        return ProjectExplorer::BuildConfiguration::Release;
    case MesonBuildType::minsize:
        return ProjectExplorer::BuildConfiguration::Release;
    default:
        return ProjectExplorer::BuildConfiguration::Unknown;
    }
}

class MesonBuildSystem;
class MesonTools;

class MesonBuildConfiguration final : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
public:
    MesonBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);
    ~MesonBuildConfiguration() final;

    static Utils::FilePath shadowBuildDirectory(
        const Utils::FilePath &projectFilePath,
        const ProjectExplorer::Kit *k,
        const QString &bcName,
        ProjectExplorer::BuildConfiguration::BuildType buildType);

    ProjectExplorer::BuildSystem *buildSystem() const final;
    void build(const QString &target);

    QStringList mesonConfigArgs();

    const QString &parameters() const;
    void setParameters(const QString &params);

signals:
    void parametersChanged();

private:
    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &map) override;
    MesonBuildType m_buildType;
    ProjectExplorer::NamedWidget *createConfigWidget() final;
    MesonBuildSystem *m_buildSystem = nullptr;
    QString m_parameters;
};

class MesonBuildConfigurationFactory final : public ProjectExplorer::BuildConfigurationFactory
{
public:
    MesonBuildConfigurationFactory();
};

} // namespace Internal
} // namespace MesonProjectManager
