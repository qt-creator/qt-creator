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

#include "projectexplorer_export.h"

#include "deploymentdata.h"
#include "kit.h"

#include <coreplugin/idocument.h>

#include <utils/environmentfwd.h>
#include <utils/fileutils.h>

#include <QObject>
#include <QFileSystemModel>

#include <functional>

namespace Core { class Context; }
namespace Utils {
class Environment;
class MacroExpander;
}

namespace ProjectExplorer {

class BuildInfo;
class BuildSystem;
class ContainerNode;
class EditorConfiguration;
class FolderNode;
class Node;
class ProjectImporter;
class ProjectNode;
class ProjectPrivate;
class Target;

// Documentation inside.
class PROJECTEXPLORER_EXPORT Project : public QObject
{
    friend class SessionManager; // for setActiveTarget
    Q_OBJECT

public:
    // Roles to be implemented by all models that are exported via model()
    enum ModelRoles {
        // Absolute file path
        FilePathRole = QFileSystemModel::FilePathRole,
        isParsingRole
    };

    Project(const QString &mimeType, const Utils::FilePath &fileName);
    ~Project() override;

    QString displayName() const;
    Utils::Id id() const;

    QString mimeType() const;
    bool canBuildProducts() const;

    BuildSystem *createBuildSystem(Target *target) const;

    Utils::FilePath projectFilePath() const;
    Utils::FilePath projectDirectory() const;
    static Utils::FilePath projectDirectory(const Utils::FilePath &top);

    // This does not affect nodes, only the root path.
    void changeRootProjectDirectory();
    Utils::FilePath rootProjectDirectory() const;

    virtual ProjectNode *rootProjectNode() const;
    ContainerNode *containerNode() const;

    // EditorConfiguration:
    EditorConfiguration *editorConfiguration() const;

    // Target:
    Target *addTargetForDefaultKit();
    Target *addTargetForKit(Kit *kit);
    bool removeTarget(Target *target);

    const QList<Target *> targets() const;
    // Note: activeTarget can be 0 (if no targets are defined).
    Target *activeTarget() const;
    Target *target(Utils::Id id) const;
    Target *target(Kit *k) const;
    virtual Tasks projectIssues(const Kit *k) const;

    static bool copySteps(Target *sourceTarget, Target *newTarget);

    void saveSettings();
    enum class RestoreResult { Ok, Error, UserAbort };
    RestoreResult restoreSettings(QString *errorMessage);

    using NodeMatcher = std::function<bool(const Node*)>;
    static const NodeMatcher AllFiles;
    static const NodeMatcher SourceFiles;
    static const NodeMatcher GeneratedFiles;

    Utils::FilePaths files(const NodeMatcher &matcher) const;
    bool isKnownFile(const Utils::FilePath &filename) const;

    virtual QVariantMap toMap() const;

    Core::Context projectContext() const;
    Core::Context projectLanguages() const;

    QVariant namedSettings(const QString &name) const;
    void setNamedSettings(const QString &name, const QVariant &value);

    void setAdditionalEnvironment(const Utils::EnvironmentItems &envItems);
    Utils::EnvironmentItems additionalEnvironment() const;

    virtual bool needsConfiguration() const;
    bool needsBuildConfigurations() const;
    virtual void configureAsExampleProject(ProjectExplorer::Kit *kit);

    virtual ProjectImporter *projectImporter() const;

    // The build system is able to report all executables that can be built, independent
    // of configuration.
    bool knowsAllBuildExecutables() const;

    virtual DeploymentKnowledge deploymentKnowledge() const { return DeploymentKnowledge::Bad; }
    bool hasMakeInstallEquivalent() const;
    virtual MakeInstallCommand makeInstallCommand(const Target *target, const QString &installRoot);

    void setup(const QList<BuildInfo> &infoList);
    Utils::MacroExpander *macroExpander() const;

    ProjectNode *findNodeForBuildKey(const QString &buildKey) const;

    bool needsInitialExpansion() const;
    void setNeedsInitialExpansion(bool needsInitialExpansion);

    void setRootProjectNode(std::unique_ptr<ProjectNode> &&root);

    // Set project files that will be watched and trigger the same callback
    // as the main project file.
    void setExtraProjectFiles(const QSet<Utils::FilePath> &projectDocumentPaths);

    void setDisplayName(const QString &name);
    void setProjectLanguage(Utils::Id id, bool enabled);
    void addProjectLanguage(Utils::Id id);

    void setExtraData(const QString &key, const QVariant &data);
    QVariant extraData(const QString &key) const;

signals:
    void projectFileIsDirty(const Utils::FilePath &path);

    void displayNameChanged();
    void fileListChanged();
    void environmentChanged();

    // Note: activeTarget can be 0 (if no targets are defined).
    void activeTargetChanged(ProjectExplorer::Target *target);

    void aboutToRemoveTarget(ProjectExplorer::Target *target);
    void removedTarget(ProjectExplorer::Target *target);
    void addedTarget(ProjectExplorer::Target *target);

    void settingsLoaded();
    void aboutToSaveSettings();

    void projectLanguagesUpdated();

    void anyParsingStarted(Target *target);
    void anyParsingFinished(Target *target, bool success);

    void rootProjectDirectoryChanged();

protected:
    virtual RestoreResult fromMap(const QVariantMap &map, QString *errorMessage);
    void createTargetFromMap(const QVariantMap &map, int index);
    virtual bool setupTarget(Target *t);

    void setCanBuildProducts();

    void setId(Utils::Id id);
    void setProjectLanguages(Core::Context language);
    void removeProjectLanguage(Utils::Id id);
    void setHasMakeInstallEquivalent(bool enabled);

    void setKnowsAllBuildExecutables(bool value);
    void setNeedsBuildConfigurations(bool value);
    void setNeedsDeployConfigurations(bool value);

    static ProjectExplorer::Task createProjectTask(ProjectExplorer::Task::TaskType type,
                                                   const QString &description);

    void setBuildSystemCreator(const std::function<BuildSystem *(Target *)> &creator);

private:
    void addTarget(std::unique_ptr<Target> &&target);

    void handleSubTreeChanged(FolderNode *node);
    void setActiveTarget(Target *target);

    friend class ContainerNode;
    ProjectPrivate *d;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Project *)
