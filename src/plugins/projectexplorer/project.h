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

#include <coreplugin/id.h>

#include <utils/fileutils.h>

#include <QObject>
#include <QFileSystemModel>

namespace Core {
class IDocument;
class Context;
}

namespace Utils { class MacroExpander; }

namespace ProjectExplorer {

class BuildInfo;
class IProjectManager;
class EditorConfiguration;
class ProjectImporter;
class ProjectNode;
class Kit;
class KitMatcher;
class NamedWidget;
class Target;
class ProjectPrivate;

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
        EnabledRole
    };

    Project();
    ~Project() override;

    virtual QString displayName() const = 0;
    Core::Id id() const;

    Core::IDocument *document() const;
    Utils::FileName projectFilePath() const;
    Utils::FileName projectDirectory() const;
    static Utils::FileName projectDirectory(const Utils::FileName &top);

    virtual IProjectManager *projectManager() const;

    virtual ProjectNode *rootProjectNode() const;

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
    virtual bool supportsKit(Kit *k, QString *errorMessage = 0) const;

    Target *createTarget(Kit *k);
    Target *cloneTarget(Target *sourceTarget, Kit *k);
    Target *restoreTarget(const QVariantMap &data);

    void saveSettings();
    enum class RestoreResult { Ok, Error, UserAbort };
    RestoreResult restoreSettings(QString *errorMessage);

    enum FilesMode {
        SourceFiles    = 0x1,
        GeneratedFiles = 0x2,
        AllFiles       = SourceFiles | GeneratedFiles
    };
    virtual QStringList files(FilesMode fileMode) const = 0;
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
    virtual ProjectImporter *createProjectImporter() const;

    KitMatcher requiredKitMatcher() const;
    void setRequiredKitMatcher(const KitMatcher &matcher);

    KitMatcher preferredKitMatcher() const;
    void setPreferredKitMatcher(const KitMatcher &matcher);

    virtual bool needsSpecialDeployment() const;

    void setup(QList<const BuildInfo *> infoList);
    Utils::MacroExpander *macroExpander() const;

signals:
    void displayNameChanged();
    void fileListChanged();

    // Note: activeTarget can be 0 (if no targets are defined).
    void activeTargetChanged(ProjectExplorer::Target *target);

    void aboutToRemoveTarget(ProjectExplorer::Target *target);
    void removedTarget(ProjectExplorer::Target *target);
    void addedTarget(ProjectExplorer::Target *target);

    void environmentChanged();
    void buildConfigurationEnabledChanged();

    void buildDirectoryChanged();

    void settingsLoaded();
    void aboutToSaveSettings();

    void projectContextUpdated();
    void projectLanguagesUpdated();

protected:
    virtual RestoreResult fromMap(const QVariantMap &map, QString *errorMessage);
    virtual bool setupTarget(Target *t);

    void setId(Core::Id id);
    void setDocument(Core::IDocument *doc); // takes ownership!
    void setProjectManager(IProjectManager *manager);
    void setRootProjectNode(ProjectNode *root); // takes ownership!
    void setProjectContext(Core::Context context);
    void setProjectLanguages(Core::Context language);
    void addProjectLanguage(Core::Id id);
    void removeProjectLanguage(Core::Id id);
    void setProjectLanguage(Core::Id id, bool enabled);
    virtual void projectLoaded(); // Called when the project is fully loaded.

private:
    void changeEnvironment();
    void changeBuildConfigurationEnabled();
    void onBuildDirectoryChanged();

    void setActiveTarget(Target *target);
    ProjectPrivate *d;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Project *)
