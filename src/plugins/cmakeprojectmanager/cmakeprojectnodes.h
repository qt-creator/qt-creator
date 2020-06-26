/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "cmakeconfigitem.h"

#include <projectexplorer/projectnodes.h>

namespace CMakeProjectManager {
namespace Internal {

class CMakeInputsNode : public ProjectExplorer::ProjectNode
{
public:
    CMakeInputsNode(const Utils::FilePath &cmakeLists);
};

class CMakeListsNode : public ProjectExplorer::ProjectNode
{
public:
    CMakeListsNode(const Utils::FilePath &cmakeListPath);

    bool showInSimpleTree() const final;
    Utils::optional<Utils::FilePath> visibleAfterAddFileAction() const override;
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

    Utils::optional<Utils::FilePath> visibleAfterAddFileAction() const override;

    void build() override;

    QVariant data(Utils::Id role) const override;
    void setConfig(const CMakeConfig &config);

private:
    QString m_tooltip;
    Utils::FilePath m_buildDirectory;
    CMakeConfig m_config;
};

} // namespace Internal
} // namespace CMakeProjectManager
