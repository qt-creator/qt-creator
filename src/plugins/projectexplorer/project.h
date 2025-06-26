// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "deploymentdata.h"
#include "kit.h"
#include "task.h"

#include <coreplugin/idocument.h>

#include <utils/environmentfwd.h>
#include <utils/filepath.h>
#include <utils/store.h>

#include <QFileSystemModel>

#include <functional>
#include <memory>

namespace Core { class Context; }
namespace TextEditor { class ProjectWrapper; }
namespace Utils {
class Environment;
class MacroExpander;
}

namespace ProjectExplorer {

class BuildConfiguration;
class BuildInfo;
class BuildSystem;
class ContainerNode;
class DeployConfiguration;
class EditorConfiguration;
class QmlCodeModelInfo;
class FolderNode;
class Node;
class ProjectImporter;
class ProjectNode;
class ProjectPrivate;
class RunConfiguration;
class Target;
enum class SetActive : int;

// Documentation inside.
class PROJECTEXPLORER_EXPORT Project : public QObject
{
    Q_OBJECT

public:
    // Roles to be implemented by all models that are exported via model()
    enum ModelRoles {
        // Absolute file path
        FilePathRole = QFileSystemModel::FilePathRole,
        isParsingRole,
        UseUnavailableMarkerRole,
    };

    Project(const QString &mimeType, const Utils::FilePath &fileName);
    ~Project() override;

    QString buildSystemName() const;
    QString displayName() const;
    Utils::Id id() const;

    void markAsShuttingDown();
    bool isShuttingDown() const;

    QString mimeType() const;
    bool canBuildProducts() const;

    BuildSystem *createBuildSystem(BuildConfiguration *bc) const;

    Utils::FilePath projectFilePath() const;
    virtual Utils::FilePath projectDirectory() const;

    // This does not affect nodes, only the root path.
    void changeRootProjectDirectory();
    Utils::FilePath rootProjectDirectory() const;

    ProjectNode *rootProjectNode() const;
    ContainerNode *containerNode() const;

    // EditorConfiguration:
    EditorConfiguration *editorConfiguration() const;

    // Target:
    Target *addTargetForDefaultKit();
    Target *addTargetForKit(Kit *kit);
    void removeTarget(Target *target);

    const QList<Target *> targets() const;
    // Note: activeTarget can be 0 (if no targets are defined).
    Target *activeTarget() const;
    Target *target(Utils::Id id) const;
    Target *target(Kit *k) const;
    void setActiveTarget(Target *target, SetActive cascade);
    void setActiveBuildConfiguration(BuildConfiguration *bc, SetActive cascade);

    Kit *activeKit() const;
    RunConfiguration *activeRunConfiguration() const;
    BuildConfiguration *activeBuildConfiguration() const;
    DeployConfiguration *activeDeployConfiguration() const;
    BuildSystem *activeBuildSystem() const;
    bool isParsing() const;

    void setIssuesGenerator(const std::function<Tasks(const Kit *)> &generator);
    virtual Tasks projectIssues(const Kit *k) const;

    static bool copySteps(Target *sourceTarget, Target *newTarget);
    bool copySteps(const Utils::Store &store, Kit *targetKit);

    void saveSettings();
    enum class RestoreResult { Ok, Error, UserAbort };
    RestoreResult restoreSettings(QString *errorMessage);

    using NodeMatcher = std::function<bool(const Node*)>;
    static const NodeMatcher AllFiles;
    static const NodeMatcher SourceFiles;
    static const NodeMatcher GeneratedFiles;
    static const NodeMatcher HiddenRccFolders;

    Utils::FilePaths files(const NodeMatcher &matcher) const;
    bool isKnownFile(const Utils::FilePath &filename) const;
    const Node *nodeForFilePath(const Utils::FilePath &filePath,
                                const NodeMatcher &extraMatcher = {}) const;
    ProjectNode *productNodeForFilePath(
        const Utils::FilePath &filePath, const NodeMatcher &extraMatcher = {}) const;
    Utils::FilePaths binariesForSourceFile(const Utils::FilePath &sourceFile) const;

    virtual void toMap(Utils::Store &map) const;

    Core::Context projectContext() const;
    Core::Context projectLanguages() const;

    QVariant namedSettings(const Utils::Key &name) const;
    void setNamedSettings(const Utils::Key &name, const QVariant &value);

    void setAdditionalEnvironment(const Utils::EnvironmentItems &envItems);
    Utils::EnvironmentItems additionalEnvironment() const;

    virtual bool needsConfiguration() const;
    bool supportsBuilding() const;
    virtual void configureAsExampleProject(ProjectExplorer::Kit *kit);

    virtual ProjectImporter *projectImporter() const;

    virtual DeploymentKnowledge deploymentKnowledge() const { return DeploymentKnowledge::Bad; }
    bool hasMakeInstallEquivalent() const;

    void setup(const QList<BuildInfo> &infoList);
    BuildConfiguration *setup(const BuildInfo &info);
    Utils::MacroExpander *macroExpander() const;

    ProjectNode *findNodeForBuildKey(const QString &buildKey) const;

    bool needsInitialExpansion() const;
    void setNeedsInitialExpansion(bool needsInitialExpansion);

    void setRootProjectNode(std::unique_ptr<ProjectNode> &&root);

    // Set project files that will be watched and by default trigger the same callback
    // as the main project file.
    using DocGenerator = std::function<std::unique_ptr<Core::IDocument>(const Utils::FilePath &)>;
    using DocUpdater = std::function<void(Core::IDocument *)>;
    void setExtraProjectFiles(const QSet<Utils::FilePath> &projectDocumentPaths,
                              const DocGenerator &docGenerator = {},
                              const DocUpdater &docUpdater = {});
    void updateExtraProjectFiles(const QSet<Utils::FilePath> &projectDocumentPaths,
                                 const DocUpdater &docUpdater);
    void updateExtraProjectFiles(const DocUpdater &docUpdater);

    void setDisplayName(const QString &name);
    void setProjectLanguage(Utils::Id id, bool enabled);

    void setExtraData(const Utils::Key &key, const QVariant &data);
    QVariant extraData(const Utils::Key &key) const;

    QStringList availableQmlPreviewTranslations(QString *errorMessage);

    QList<Core::IDocument *> modifiedDocuments() const;
    bool isModified() const;

    bool isEditModePreferred() const;

    void registerGenerator(Utils::Id id, const QString &displayName,
                           const std::function<void()> &runner);
    const QList<QPair<Utils::Id, QString>> allGenerators() const;
    void runGenerator(Utils::Id id);

    static void addVariablesToMacroExpander(const QByteArray &prefix,
                                            const QString &descriptor,
                                            Utils::MacroExpander *expander,
                                            const std::function<Project *()> &projectGetter);
    static Task createTask(ProjectExplorer::Task::TaskType type, const QString &description);

    QList<Utils::Store> vanishedTargets() const;
    void removeVanishedTarget(int index);
    void removeAllVanishedTargets();
    Target *createKitAndTargetFromStore(const Utils::Store &store);

    void collectQmlCodeModelInfo(QmlCodeModelInfo &projectInfo, Kit *kit);

    using QmlCodeModelInfoFromQtVersionHook = void (*)(Kit *, QmlCodeModelInfo &);
    static void setQmlCodeModelInfoFromQtVersionHook(QmlCodeModelInfoFromQtVersionHook hook);

    static void setQmlCodeModelIsUsed();
    void updateQmlCodeModel(Kit *kit, BuildConfiguration *bc);
    QmlCodeModelInfo gatherQmlCodeModelInfo(Kit *kit, BuildConfiguration *bc);

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

    void activeBuildConfigurationChanged(BuildConfiguration *bc);

    void vanishedTargetsChanged();

    void settingsLoaded();
    void aboutToSaveSettings();

    void projectLanguagesUpdated();

    void anyParsingStarted();
    void anyParsingFinished(bool success);

    void rootProjectDirectoryChanged();

#ifdef WITH_TESTS
    void indexingFinished(Utils::Id indexer);
#endif

protected:
    virtual RestoreResult fromMap(const Utils::Store &map, QString *errorMessage);
    void createTargetFromMap(const Utils::Store &map, int index);

    void setCanBuildProducts();
    void setIsEditModePreferred(bool preferEditMode);

    void setId(Utils::Id id);
    void setProjectLanguages(Core::Context language);
    void setHasMakeInstallEquivalent(bool enabled);

    void setSupportsBuilding(bool value);

    template <typename BuildSystemImpl>
    void setBuildSystemCreator(const QString &name) {
        setBuildSystemName(name);
        setBuildSystemCreator([](BuildConfiguration *bc) { return new BuildSystemImpl(bc); });
    }

private:
    void setBuildSystemName(const QString &name);
    void setBuildSystemCreator(const std::function<BuildSystem *(BuildConfiguration *)> &creator);

    void addTarget(std::unique_ptr<Target> &&target);

    void addProjectLanguage(Utils::Id id);
    void removeProjectLanguage(Utils::Id id);

    void handleSubTreeChanged(FolderNode *node);
    void setActiveTargetHelper(Target *target);

    void handleKitUpdated(Kit *k);
    void handleKitRemoval(Kit *k);

    friend class ContainerNode;
    ProjectPrivate *d;
};

PROJECTEXPLORER_EXPORT TextEditor::ProjectWrapper wrapProject(Project *p);
PROJECTEXPLORER_EXPORT Project *unwrapProject(const TextEditor::ProjectWrapper &w);

#ifdef WITH_TESTS
namespace Internal { QObject *createProjectTest(); }
#endif

} // namespace ProjectExplorer
