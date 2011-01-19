/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
}

namespace ProjectExplorer {

class BuildConfigWidget;
class IProjectManager;
class EditorConfiguration;
class ProjectNode;
class Target;
class ITargetFactory;
class ProjectPrivate;

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

    // Serialize all data into a QVariantMap. This map is then saved
    // in the .user file of the project.
    //
    // Just put all your data into the map.
    //
    // Note: Do not forget to call your base class' toMap method.
    // Note: Do not forget to call setActiveBuildConfiguration when
    //       creating new BuilConfigurations.
    virtual QVariantMap toMap() const;

    // The directory that holds the project file. This includes the absolute path.
    QString projectDirectory() const;
    static QString projectDirectory(const QString &proFile);

signals:
    void fileListChanged();

    // Note: activeTarget can be 0 (if no targets are defined).
    void activeTargetChanged(ProjectExplorer::Target *target);

    void aboutToRemoveTarget(ProjectExplorer::Target *target);
    void removedTarget(ProjectExplorer::Target *target);
    void addedTarget(ProjectExplorer::Target *target);

    /// convenience signal emitted if the activeBuildConfiguration emits environmentChanged
    /// or if the activeBuildConfiguration changes
    /// (including due to the active target changing).
    void environmentChanged();

    /// convenience signal emitted if the activeBuildConfiguration emits isEnabledChanged()
    /// or if the activeBuildConfiguration changes
    /// (including due to the active target changing).
    void buildConfigurationEnabledChanged();

protected:
    // restore all data from the map.
    //
    // Note: Do not forget to call your base class' fromMap method!
    virtual bool fromMap(const QVariantMap &map);

private slots:
    void changeEnvironment();
    void changeBuildConfigurationEnabled();

private:
    QScopedPointer<ProjectPrivate> d;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Project *)

#endif // PROJECT_H
