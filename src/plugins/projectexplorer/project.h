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
#include "subscription.h"

#include <coreplugin/id.h>
#include <coreplugin/idocument.h>

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
class NamedWidget;
class Node;
class ProjectConfiguration;
class ProjectImporter;
class ProjectNode;
class ProjectPrivate;
class Target;

// Documentation inside.
class PROJECTEXPLORER_EXPORT Project : public QObject
{
    friend class SessionManager; // for setActiveTarget
    friend class ProjectExplorerPlugin; // for projectLoaded
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
    Core::Id id() const;

    QString mimeType() const;
    bool canBuildProducts() const;

    BuildSystem *buildSystem() const;

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

    QList<Target *> targets() const;
    // Note: activeTarget can be 0 (if no targets are defined).
    Target *activeTarget() const;
    Target *target(Core::Id id) const;
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

    Utils::FilePathList files(const NodeMatcher &matcher) const;
    virtual QStringList filesGeneratedFrom(const QString &sourceFile) const;
    bool isKnownFile(const Utils::FilePath &filename) const;

    virtual QVariantMap toMap() const;

    Core::Context projectContext() const;
    Core::Context projectLanguages() const;

    QVariant namedSettings(const QString &name) const;
    void setNamedSettings(const QString &name, const QVariant &value);

    virtual bool needsConfiguration() const;
    bool needsBuildConfigurations() const;
    virtual void configureAsExampleProject();

    virtual ProjectImporter *projectImporter() const;

    Kit::Predicate requiredKitPredicate() const;
    Kit::Predicate preferredKitPredicate() const;

    // The build system is able to report all executables that can be built, independent
    // of configuration.
    bool knowsAllBuildExecutables() const;

    virtual DeploymentKnowledge deploymentKnowledge() const { return DeploymentKnowledge::Bad; }
    bool hasMakeInstallEquivalent() const;
    virtual MakeInstallCommand makeInstallCommand(const Target *target, const QString &installRoot);

    void setup(const QList<BuildInfo> &infoList);
    Utils::MacroExpander *macroExpander() const;

    virtual QVariant additionalData(Core::Id id, const Target *target) const;

    bool isParsing() const;
    bool hasParsingData() const;

    ProjectNode *findNodeForBuildKey(const QString &buildKey) const;

    template<typename S, typename R, typename T, typename ...Args1, typename ...Args2>
    void subscribeSignal(void (S::*sig)(Args1...), R*recv, T (R::*sl)(Args2...)) {
        new Internal::ProjectSubscription([sig, recv, sl, this](ProjectConfiguration *pc) {
            if (S* sender = qobject_cast<S*>(pc))
                return connect(sender, sig, recv, sl);
            return QMetaObject::Connection();
        }, recv, this);
    }

    template<typename S, typename R, typename T, typename ...Args1>
    void subscribeSignal(void (S::*sig)(Args1...), R*recv, T sl) {
        new Internal::ProjectSubscription([sig, recv, sl, this](ProjectConfiguration *pc) {
            if (S* sender = qobject_cast<S*>(pc))
                return connect(sender, sig, recv, sl);
            return QMetaObject::Connection();
        }, recv, this);
    }

    bool needsInitialExpansion() const;
    void setNeedsInitialExpansion(bool needsInitialExpansion);

    class ParseGuard
    {
    public:
        ParseGuard()
            : ParseGuard(nullptr)
        {}

        ~ParseGuard()
        {
            if (m_project)
                m_project->emitParsingFinished(m_success);
        }

        void markAsSuccess() const { m_success = true; }
        bool isSuccess() const { return m_success; }
        bool isNull() const { return !m_project; }

        ParseGuard(const ParseGuard &other) = delete;
        ParseGuard &operator=(const ParseGuard &other) = delete;
        ParseGuard(ParseGuard &&other)
        {
            std::swap(m_project, other.m_project);
            std::swap(m_success, other.m_success);
        }
        ParseGuard &operator=(ParseGuard &&other)
        {
            std::swap(m_project, other.m_project);
            std::swap(m_success, other.m_success);
            return *this;
        }

    private:
        ParseGuard(Project *p)
            : m_project(p)
        {
            if (m_project)
                m_project->emitParsingStarted();
        }

        Project *m_project = nullptr;
        mutable bool m_success = false;

        friend class Project;
    };

    // FIXME: Make this private and the BuildSystem a friend
    ParseGuard guardParsingRun() { return ParseGuard(this); }
    void setRootProjectNode(std::unique_ptr<ProjectNode> &&root);

    // Set project files that will be watched and trigger the same callback
    // as the main project file.
    void setExtraProjectFiles(const QVector<Utils::FilePath> &projectDocumentPaths);

    Utils::Environment activeParseEnvironment() const;

signals:
    void projectFileIsDirty(const Utils::FilePath &path);

    void displayNameChanged();
    void fileListChanged();

    // Note: activeTarget can be 0 (if no targets are defined).
    void activeTargetChanged(ProjectExplorer::Target *target);

    void removedProjectConfiguration(ProjectExplorer::ProjectConfiguration *pc);
    void addedProjectConfiguration(ProjectExplorer::ProjectConfiguration *pc);

    // *ANY* active build configuration changed somewhere in the tree. This might not be
    // the one that would get started right now, since some part of the tree in between might
    // not be active.
    void activeBuildConfigurationChanged(ProjectExplorer::ProjectConfiguration *bc);

    void aboutToRemoveTarget(ProjectExplorer::Target *target);
    void removedTarget(ProjectExplorer::Target *target);
    void addedTarget(ProjectExplorer::Target *target);

    void settingsLoaded();
    void aboutToSaveSettings();

    void projectLanguagesUpdated();

    void parsingStarted();
    void parsingFinished(bool success);

    void rootProjectDirectoryChanged();

protected:
    virtual RestoreResult fromMap(const QVariantMap &map, QString *errorMessage);
    void createTargetFromMap(const QVariantMap &map, int index);
    virtual bool setupTarget(Target *t);

    void setDisplayName(const QString &name);
    // Used to pre-check kits in the TargetSetupPage. RequiredKitPredicate
    // is used to select kits available in the TargetSetupPage
    void setPreferredKitPredicate(const Kit::Predicate &predicate);
    // The predicate used to select kits available in TargetSetupPage.
    void setRequiredKitPredicate(const Kit::Predicate &predicate);

    void setCanBuildProducts();

    void setId(Core::Id id);
    void setProjectLanguages(Core::Context language);
    void addProjectLanguage(Core::Id id);
    void removeProjectLanguage(Core::Id id);
    void setProjectLanguage(Core::Id id, bool enabled);
    void setHasMakeInstallEquivalent(bool enabled);
    virtual void projectLoaded(); // Called when the project is fully loaded.

    void setKnowsAllBuildExecutables(bool value);
    void setNeedsBuildConfigurations(bool value);

    static ProjectExplorer::Task createProjectTask(ProjectExplorer::Task::TaskType type,
                                                   const QString &description);

    void setBuildSystem(std::unique_ptr<BuildSystem> &&bs); // takes ownership!

private:
    // Helper methods to manage parsing state and signalling
    // Call in GUI thread before the actual parsing starts
    void emitParsingStarted();
    // Call in GUI thread right after the actual parsing is done
    void emitParsingFinished(bool success);

    void addTarget(std::unique_ptr<Target> &&target);

    void handleSubTreeChanged(FolderNode *node);
    void setActiveTarget(Target *target);

    ProjectPrivate *d;

    friend class ContainerNode;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Project *)
