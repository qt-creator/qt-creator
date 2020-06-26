/****************************************************************************
**
** Copyright (C) 2016 Canonical Ltd.
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

#include "cmake_global.h"

#include "cmakeconfigitem.h"

#include <projectexplorer/kitmanager.h>

namespace CMakeProjectManager {
class CMakeTool;

class CMAKE_EXPORT CMakeKitAspect : public ProjectExplorer::KitAspect
{
    Q_OBJECT
public:
    CMakeKitAspect();

    static Utils::Id id();

    static Utils::Id cmakeToolId(const ProjectExplorer::Kit *k);
    static CMakeTool *cmakeTool(const ProjectExplorer::Kit *k);
    static void setCMakeTool(ProjectExplorer::Kit *k, const Utils::Id id);

    // KitAspect interface
    ProjectExplorer::Tasks validate(const ProjectExplorer::Kit *k) const final;
    void setup(ProjectExplorer::Kit *k) final;
    void fix(ProjectExplorer::Kit *k) final;
    ItemList toUserOutput(const ProjectExplorer::Kit *k) const final;
    ProjectExplorer::KitAspectWidget *createConfigWidget(ProjectExplorer::Kit *k) const final;

    void addToMacroExpander(ProjectExplorer::Kit *k, Utils::MacroExpander *expander) const final;

    QSet<Utils::Id> availableFeatures(const ProjectExplorer::Kit *k) const final;
};

class CMAKE_EXPORT CMakeGeneratorKitAspect : public ProjectExplorer::KitAspect
{
    Q_OBJECT
public:
    CMakeGeneratorKitAspect();

    static QString generator(const ProjectExplorer::Kit *k);
    static QString extraGenerator(const ProjectExplorer::Kit *k);
    static QString platform(const ProjectExplorer::Kit *k);
    static QString toolset(const ProjectExplorer::Kit *k);
    static void setGenerator(ProjectExplorer::Kit *k, const QString &generator);
    static void setExtraGenerator(ProjectExplorer::Kit *k, const QString &extraGenerator);
    static void setPlatform(ProjectExplorer::Kit *k, const QString &platform);
    static void setToolset(ProjectExplorer::Kit *k, const QString &toolset);
    static void set(ProjectExplorer::Kit *k, const QString &generator,
                    const QString &extraGenerator, const QString &platform, const QString &toolset);
    static QStringList generatorArguments(const ProjectExplorer::Kit *k);

    // KitAspect interface
    ProjectExplorer::Tasks validate(const ProjectExplorer::Kit *k) const final;
    void setup(ProjectExplorer::Kit *k) final;
    void fix(ProjectExplorer::Kit *k) final;
    void upgrade(ProjectExplorer::Kit *k) final;
    ItemList toUserOutput(const ProjectExplorer::Kit *k) const final;
    ProjectExplorer::KitAspectWidget *createConfigWidget(ProjectExplorer::Kit *k) const final;
    void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const final;

private:
    QVariant defaultValue(const ProjectExplorer::Kit *k) const;
};

class CMAKE_EXPORT CMakeConfigurationKitAspect : public ProjectExplorer::KitAspect
{
    Q_OBJECT
public:
    CMakeConfigurationKitAspect();

    static CMakeConfig configuration(const ProjectExplorer::Kit *k);
    static void setConfiguration(ProjectExplorer::Kit *k, const CMakeConfig &config);

    static QStringList toStringList(const ProjectExplorer::Kit *k);
    static void fromStringList(ProjectExplorer::Kit *k, const QStringList &in);

    static QStringList toArgumentsList(const ProjectExplorer::Kit *k);

    static CMakeConfig defaultConfiguration(const ProjectExplorer::Kit *k);

    // KitAspect interface
    ProjectExplorer::Tasks validate(const ProjectExplorer::Kit *k) const final;
    void setup(ProjectExplorer::Kit *k) final;
    void fix(ProjectExplorer::Kit *k) final;
    ItemList toUserOutput(const ProjectExplorer::Kit *k) const final;
    ProjectExplorer::KitAspectWidget *createConfigWidget(ProjectExplorer::Kit *k) const final;

private:
    QVariant defaultValue(const ProjectExplorer::Kit *k) const;
};

} // namespace CMakeProjectManager
