// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>
#include <utils/id.h>
#include <utils/parameteraction.h>

namespace ProjectExplorer {
class Target;
class Node;
} // namespace ProjectExplorer

namespace QbsProjectManager {
namespace Internal {

class QbsProject;
class QbsProjectManagerPluginPrivate;

class QbsProjectManagerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QbsProjectManager.json")

public:
    static void buildNamedProduct(QbsProject *project, const QString &product);

private:
    ~QbsProjectManagerPlugin() final;

    void initialize() final;

    void targetWasAdded(ProjectExplorer::Target *target);
    void projectChanged(QbsProject *project);

    void buildFileContextMenu();
    void buildFile();
    void buildProductContextMenu();
    void cleanProductContextMenu();
    void rebuildProductContextMenu();
    void runStepsForProductContextMenu(const QList<Utils::Id> &stepTypes);
    void buildProduct();
    void cleanProduct();
    void rebuildProduct();
    void runStepsForProduct(const QList<Utils::Id> &stepTypes);
    void buildSubprojectContextMenu();
    void cleanSubprojectContextMenu();
    void rebuildSubprojectContextMenu();
    void runStepsForSubprojectContextMenu(const QList<Utils::Id> &stepTypes);

    void reparseSelectedProject();
    void reparseCurrentProject();
    void reparseProject(QbsProject *project);

    void updateContextActions(ProjectExplorer::Node *node);
    void updateReparseQbsAction();
    void updateBuildActions();

    void buildFiles(QbsProject *project, const QStringList &files,
                    const QStringList &activeFileTags);
    void buildSingleFile(QbsProject *project, const QString &file);

    static void runStepsForProducts(QbsProject *project,
                                    const QStringList &products,
                                    const QList<Utils::Id> &stepTypes);

    QbsProjectManagerPluginPrivate *d = nullptr;
    QAction *m_reparseQbs = nullptr;
    QAction *m_reparseQbsCtx = nullptr;
    QAction *m_buildFileCtx = nullptr;
    QAction *m_buildProductCtx = nullptr;
    QAction *m_cleanProductCtx = nullptr;
    QAction *m_rebuildProductCtx = nullptr;
    QAction *m_buildSubprojectCtx = nullptr;
    QAction *m_cleanSubprojectCtx = nullptr;
    QAction *m_rebuildSubprojectCtx = nullptr;
    Utils::ParameterAction *m_buildFile = nullptr;
    Utils::ParameterAction *m_buildProduct = nullptr;
    QAction *m_cleanProduct = nullptr;
    QAction *m_rebuildProduct = nullptr;
};

} // namespace Internal
} // namespace QbsProjectManager
