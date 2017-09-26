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

#include "kit.h"
#include "subscription.h"

#include <coreplugin/id.h>
#include <coreplugin/idocument.h>

#include <utils/fileutils.h>

#include <QObject>
#include <QFileSystemModel>

#include <functional>

namespace Core { class Context; }
namespace Utils { class MacroExpander; }

namespace ProjectExplorer {

class BuildInfo;
class ContainerNode;
class EditorConfiguration;
class NamedWidget;
class Node;
class ProjectConfiguration;
class ProjectImporter;
class ProjectNode;
class ProjectPrivate;
class Session;
class Target;

// Auto-registers with the DocumentManager if a callback is set!
class PROJECTEXPLORER_EXPORT ProjectDocument : public Core::IDocument
{
public:
    using ProjectCallback = std::function<void()>;

    ProjectDocument(const QString &mimeType, const Utils::FileName &fileName,
                    const ProjectCallback &callback = {});

    Core::IDocument::ReloadBehavior reloadBehavior(Core::IDocument::ChangeTrigger state,
                                                   Core::IDocument::ChangeType type) const final;
    bool reload(QString *errorString, Core::IDocument::ReloadFlag flag,
                Core::IDocument::ChangeType type) final;

private:
    ProjectCallback m_callback;
};

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
        EnabledRole,
        isParsingRole
    };

    Project(const QString &mimeType, const Utils::FileName &fileName,
            const ProjectDocument::ProjectCallback &callback = {});
    ~Project() override;

    QString displayName() const;
    Core::Id id() const;

    Core::IDocument *document() const;
    Utils::FileName projectFilePath() const;
    Utils::FileName projectDirectory() const;
    static Utils::FileName projectDirectory(const Utils::FileName &top);

    virtual ProjectNode *rootProjectNode() const;
    ContainerNode *containerNode() const;

    bool hasActiveBuildSettings() const;

    // EditorConfiguration:
    EditorConfiguration *editorConfiguration() const;

    // Target:
    void addTarget(Target *target);
    bool removeTarget(Target *target);

    QList<Target *> targets() const;
    // Note: activeTarget can be 0 (if no targets are defined).
    Target *activeTarget() const;
    Target *target(Core::Id id) const;
    Target *target(Kit *k) const;
    virtual bool supportsKit(Kit *k, QString *errorMessage = nullptr) const;

    Target *createTarget(Kit *k);
    static bool copySteps(Target *sourceTarget, Target *newTarget);
    Target *restoreTarget(const QVariantMap &data);

    void saveSettings();
    enum class RestoreResult { Ok, Error, UserAbort };
    RestoreResult restoreSettings(QString *errorMessage);

    enum FilesMode {
        SourceFiles    = 0x1,
        GeneratedFiles = 0x2,
        AllFiles       = SourceFiles | GeneratedFiles
    };
    QStringList files(FilesMode fileMode,
                      const std::function<bool(const Node *)> &filter = {}) const;
    virtual QStringList filesGeneratedFrom(const QString &sourceFile) const;

    static QString makeUnique(const QString &preferredName, const QStringList &usedNames);

    virtual QVariantMap toMap() const;

    Core::Context projectContext() const;
    Core::Context projectLanguages() const;

    QVariant namedSettings(const QString &name) const;
    void setNamedSettings(const QString &name, const QVariant &value);

    virtual bool needsConfiguration() const;
    virtual void configureAsExampleProject(const QSet<Core::Id> &platforms);

    virtual bool requiresTargetPanel() const;
    virtual ProjectImporter *projectImporter() const;

    Kit::Predicate requiredKitPredicate() const;
    Kit::Predicate preferredKitPredicate() const;

    virtual bool needsSpecialDeployment() const;
    // The build system is able to report all executables that can be built, independent
    // of configuration.
    virtual bool knowsAllBuildExecutables() const;

    void setup(QList<const BuildInfo *> infoList);
    Utils::MacroExpander *macroExpander() const;

    bool isParsing() const;
    bool hasParsingData() const;

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

signals:
    void displayNameChanged();
    void fileListChanged();

    // Note: activeTarget can be 0 (if no targets are defined).
    void activeTargetChanged(ProjectExplorer::Target *target);

    void aboutToRemoveProjectConfiguration(ProjectExplorer::ProjectConfiguration *pc);
    void removedProjectConfiguration(ProjectExplorer::ProjectConfiguration *pc);
    void addedProjectConfiguration(ProjectExplorer::ProjectConfiguration *pc);

    // *ANY* active project configuration changed somewhere in the tree. This might not be
    // the one that would get started right now, since some part of the tree in between might
    // not be active.
    void activeProjectConfigurationChanged(ProjectExplorer::ProjectConfiguration *pc);

    void aboutToRemoveTarget(ProjectExplorer::Target *target);
    void removedTarget(ProjectExplorer::Target *target);
    void addedTarget(ProjectExplorer::Target *target);

    void settingsLoaded();
    void aboutToSaveSettings();

    void projectContextUpdated();
    void projectLanguagesUpdated();

    void parsingStarted();
    void parsingFinished(bool success);

protected:
    virtual RestoreResult fromMap(const QVariantMap &map, QString *errorMessage);
    void createTargetFromMap(const QVariantMap &map, int index);
    virtual bool setupTarget(Target *t);

    // Helper methods to manage parsing state and signalling
    // Call in GUI thread before the actual parsing starts
    void emitParsingStarted();
    // Call in GUI thread right after the actual parsing is done
    void emitParsingFinished(bool success);

    void setDisplayName(const QString &name);
    void setRequiredKitPredicate(const Kit::Predicate &predicate);
    void setPreferredKitPredicate(const Kit::Predicate &predicate);

    void setId(Core::Id id);
    void setRootProjectNode(ProjectNode *root); // takes ownership!
    void setProjectContext(Core::Context context);
    void setProjectLanguages(Core::Context language);
    void addProjectLanguage(Core::Id id);
    void removeProjectLanguage(Core::Id id);
    void setProjectLanguage(Core::Id id, bool enabled);
    virtual void projectLoaded(); // Called when the project is fully loaded.

private:
    void setActiveTarget(Target *target);
    ProjectPrivate *d;

    friend class Session;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Project *)
