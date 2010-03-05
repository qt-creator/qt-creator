/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef PROJECT_H
#define PROJECT_H

#include "projectexplorer_export.h"
#include "target.h"

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

    virtual bool isApplication() const = 0;
    bool hasActiveBuildSettings() const;

    // EditorConfiguration:
    EditorConfiguration *editorConfiguration() const;

    // Target:

    // Note: You can only add a specific kind of target (identified by id)
    //       once.
    QSet<QString> supportedTargetIds() const;
    QSet<QString> possibleTargetIds() const;
    bool canAddTarget(const QString &id) const;

    void addTarget(Target *target);
    void removeTarget(Target *target);

    QList<Target *> targets() const;
    // Note: activeTarget can be 0 (if no targets are defined).
    Target *activeTarget() const;
    void setActiveTarget(Target *target);
    Target *target(const QString &id) const;

    virtual ITargetFactory *targetFactory() const = 0;

    void saveSettings();
    bool restoreSettings();

    virtual BuildConfigWidget *createConfigWidget() = 0;
    virtual QList<BuildConfigWidget*> subConfigWidgets();

    virtual ProjectNode *rootProjectNode() const = 0;

    enum FilesMode { AllFiles, ExcludeGeneratedFiles };
    virtual QStringList files(FilesMode fileMode) const = 0;
    // TODO: generalize to find source(s) of generated files?
    virtual QString generatedUiHeader(const QString &formFile) const;

    static QString makeUnique(const QString &preferedName, const QStringList &usedNames);

    // C++ specific
    // TODO do a C++ project as a base ?
    virtual QByteArray predefinedMacros(const QString &fileName) const;
    virtual QStringList includePaths(const QString &fileName) const;
    virtual QStringList frameworkPaths(const QString &fileName) const;

    // Serialize all data into a QVariantMap. This map is then saved
    // in the .user file of the project.
    //
    // Just put all your data into the map.
    //
    // Note: Do not forget to call your base class' toMap method.
    // Note: Do not forget to call setActiveBuildConfiguration when
    //       creating new BuilConfigurations.
    virtual QVariantMap toMap() const;

signals:
    void fileListChanged();

    // Note: activeTarget can be 0 (if no targets are defined).
    void activeTargetChanged(ProjectExplorer::Target *target);

    void aboutToRemoveTarget(ProjectExplorer::Target *target);
    void removedTarget(ProjectExplorer::Target *target);
    void addedTarget(ProjectExplorer::Target *target);

    void supportedTargetIdsChanged();

    /// convenience signal emitted if the activeBuildConfiguration emits environmentChanged
    /// or if the activeBuildConfiguration changes
    /// (which theoretically might happen due to the active target changing).
    void environmentChanged();

protected:
    // restore all data from the map.
    //
    // Note: Do not forget to call your base class' fromMap method!
    virtual bool fromMap(const QVariantMap &map);

    void setSupportedTargetIds(const QSet<QString> &ids);

private slots:
    void changeEnvironment();

private:
    QSet<QString> m_supportedTargetIds;
    QList<Target *> m_targets;
    Target *m_activeTarget;
    EditorConfiguration *m_editorConfiguration;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Project *)

#endif // PROJECT_H
