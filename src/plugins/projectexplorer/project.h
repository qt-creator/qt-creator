/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef PROJECT_H
#define PROJECT_H

#include "projectexplorer_export.h"

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtGui/QFileSystemModel>

namespace Core {
class IFile;
class Context;
}

namespace ProjectExplorer {

class BuildConfigWidget;
class IProjectManager;
class EditorConfiguration;
class ProjectNode;
class Target;
class ProjectPrivate;

// Documentation inside.
class PROJECTEXPLORER_EXPORT Project
    : public QObject
{
    Q_OBJECT

public:
    // Roles to be implemented by all models that are exported via model()
    enum ModelRoles {
        // Absolute file path
        FilePathRole = QFileSystemModel::FilePathRole
    };

    Project();
    virtual ~Project();

    virtual QString displayName() const = 0;
    virtual QString id() const = 0;
    virtual Core::IFile *file() const = 0;
    virtual IProjectManager *projectManager() const = 0;

    virtual QList<Project *> dependsOn() = 0; //NBS TODO implement dependsOn

    bool hasActiveBuildSettings() const;

    // EditorConfiguration:
    EditorConfiguration *editorConfiguration() const;

    // Target:
    void addTarget(Target *target);
    void removeTarget(Target *target);

    QList<Target *> targets() const;
    // Note: activeTarget can be 0 (if no targets are defined).
    Target *activeTarget() const;
    void setActiveTarget(Target *target);
    Target *target(const QString &id) const;

    void saveSettings();
    bool restoreSettings();

    virtual QList<BuildConfigWidget*> subConfigWidgets();

    virtual ProjectNode *rootProjectNode() const = 0;

    enum FilesMode { AllFiles, ExcludeGeneratedFiles };
    virtual QStringList files(FilesMode fileMode) const = 0;
    // TODO: generalize to find source(s) of generated files?
    virtual QString generatedUiHeader(const QString &formFile) const;

    static QString makeUnique(const QString &preferedName, const QStringList &usedNames);

    virtual QVariantMap toMap() const;

    // The directory that holds the project file. This includes the absolute path.
    QString projectDirectory() const;
    static QString projectDirectory(const QString &proFile);

    virtual Core::Context projectContext() const;
    virtual Core::Context projectLanguage() const;

    QVariant namedSettings(const QString &name) const;
    void setNamedSettings(const QString &name, QVariant &value);

signals:
    void fileListChanged();

    // Note: activeTarget can be 0 (if no targets are defined).
    void activeTargetChanged(ProjectExplorer::Target *target);

    void aboutToRemoveTarget(ProjectExplorer::Target *target);
    void removedTarget(ProjectExplorer::Target *target);
    void addedTarget(ProjectExplorer::Target *target);

    void environmentChanged();
    void buildConfigurationEnabledChanged();

    void settingsLoaded();
    void aboutToSaveSettings();

protected:
    // The methods toMap/fromMap are used for serialization of settings data. By default, all
    // settings are saved in the .user file. For shared settings, one needs to explicitly
    // specify which keys from a particular map should be saved in the .shared file. This is
    // done in the following way:
    //
    //  - Create an item in the map with the key SHARED_SETTINGS_KEYS_KEY and a QStringList
    //    as a value. If everything from this particular map should be shared simply add
    //    the unique item ALL_SETTINGS_KEYS_KEY to the QStringList. Otherwise, add to the
    //    QStringList the keys that should be shared.
    //
    // Notice that the sharing process is smart enough in terms of recursion and grouping of
    // keys. This means that shared keys from deeply nested maps don't need to be propagated
    // anyhow to the top-level map. Simply add them from within the map they actually belong.
    //
    // The other thing to notice is that shared keys are not really excluded from the user
    // settings file. More details about that in the SettingsAcessor.
    virtual bool fromMap(const QVariantMap &map);

    virtual void setProjectContext(Core::Context context);
    virtual void setProjectLanguage(Core::Context language);

private slots:
    void changeEnvironment();
    void changeBuildConfigurationEnabled();

private:
    ProjectPrivate *d;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Project *)

#endif // PROJECT_H
