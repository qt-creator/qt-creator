// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakeconfigitem.h"

#include <projectexplorer/projectnodes.h>

namespace CMakeProjectManager::Internal {

class CMakeInputsNode : public ProjectExplorer::ProjectNode
{
public:
    CMakeInputsNode(const Utils::FilePath &cmakeLists);
};

class CMakePresetsNode : public ProjectExplorer::ProjectNode
{
public:
    CMakePresetsNode(const Utils::FilePath &projectPath);
};

class CMakeListsNode : public ProjectExplorer::ProjectNode
{
public:
    CMakeListsNode(const Utils::FilePath &cmakeListPath);

    bool showInSimpleTree() const final;
    std::optional<Utils::FilePath> visibleAfterAddFileAction() const override;
};

class CMakeProjectNode : public ProjectExplorer::ProjectNode
{
public:
    CMakeProjectNode(const Utils::FilePath &directory);

    QString tooltip() const final;
};

class CMakeTargetNode : public ProjectExplorer::ProjectNode
{
public:
    CMakeTargetNode(const Utils::FilePath &directory, const QString &target);

    void setTargetInformation(const QList<Utils::FilePath> &artifacts, const QString &type);

    QString tooltip() const final;
    QString buildKey() const final;
    Utils::FilePath buildDirectory() const;
    void setBuildDirectory(const Utils::FilePath &directory);

    std::optional<Utils::FilePath> visibleAfterAddFileAction() const override;

    void build() override;

    QVariant data(Utils::Id role) const override;
    void setConfig(const CMakeConfig &config);

private:
    QString m_tooltip;
    Utils::FilePath m_buildDirectory;
    Utils::FilePath m_artifact;
    CMakeConfig m_config;
};

} // CMakeProjectManager::Internal
