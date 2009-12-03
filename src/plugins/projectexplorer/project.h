/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtCore/QObject>
#include <QtGui/QFileSystemModel>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace Core {
class IFile;
}

namespace ProjectExplorer {

class BuildManager;
class BuildStep;
class BuildConfigWidget;
class IProjectManager;
class RunConfiguration;
class EditorConfiguration;
class Environment;
class ProjectNode;
class PersistentSettingsWriter;
class PersistentSettingsReader;
class BuildConfiguration;
class IBuildConfigurationFactory;

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

    virtual QString name() const = 0;
    virtual Core::IFile *file() const = 0;
    virtual IProjectManager *projectManager() const = 0;

    virtual QList<Project *> dependsOn() = 0; //NBS TODO implement dependsOn

    virtual bool isApplication() const = 0;

    virtual bool hasBuildSettings() const;

    // Build configuration
    void addBuildConfiguration(BuildConfiguration *configuration);
    void removeBuildConfiguration(BuildConfiguration *configuration);

    QList<BuildConfiguration *> buildConfigurations() const;
    BuildConfiguration *activeBuildConfiguration() const;
    void setActiveBuildConfiguration(BuildConfiguration *configuration);

    virtual IBuildConfigurationFactory *buildConfigurationFactory() const = 0;

    // Running
    QList<RunConfiguration *> runConfigurations() const;
    void addRunConfiguration(RunConfiguration* runConfiguration);
    void removeRunConfiguration(RunConfiguration* runConfiguration);

    RunConfiguration* activeRunConfiguration() const;
    void setActiveRunConfiguration(RunConfiguration* runConfiguration);

    EditorConfiguration *editorConfiguration() const;

    void saveSettings();
    bool restoreSettings();

    virtual BuildConfigWidget *createConfigWidget() = 0;
    virtual QList<BuildConfigWidget*> subConfigWidgets();

    virtual ProjectNode *rootProjectNode() const = 0;

    enum FilesMode { AllFiles, ExcludeGeneratedFiles };
    virtual QStringList files(FilesMode fileMode) const = 0;
    // TODO: generalize to find source(s) of generated files?
    virtual QString generatedUiHeader(const QString &formFile) const;

    // C++ specific
    // TODO do a C++ project as a base ?
    virtual QByteArray predefinedMacros(const QString &fileName) const;
    virtual QStringList includePaths(const QString &fileName) const;
    virtual QStringList frameworkPaths(const QString &fileName) const;

    static QString makeUnique(const QString &preferedName, const QStringList &usedNames);
signals:
    void fileListChanged();

// TODO clean up signal names
// might be better to also have
// a aboutToRemoveRunConfiguration
// and a removedBuildConfiguration
// a runconfiguration display name changed is missing
    void activeBuildConfigurationChanged();
    void activeRunConfigurationChanged();
    void runConfigurationsEnabledStateChanged();

    void removedRunConfiguration(ProjectExplorer::Project *p, const QString &name);
    void addedRunConfiguration(ProjectExplorer::Project *p, const QString &name);

    void removedBuildConfiguration(ProjectExplorer::Project *p, ProjectExplorer::BuildConfiguration *bc);
    void addedBuildConfiguration(ProjectExplorer::Project *p, ProjectExplorer::BuildConfiguration *bc);

protected:
    /* This method is called when the project .user file is saved. Simply call
     * writer.saveValue() for each value you want to save. Make sure to always
     * call your base class implementation.
     *
     * Note: All the values from the project/buildsteps and buildconfigurations
     * are automatically stored.
     */
    virtual void saveSettingsImpl(PersistentSettingsWriter &writer);

    /* This method is called when the project is opened. You can retrieve all
     * the values you saved in saveSettingsImpl() in this method.
     *
     * Note: This function is also called if there is no .user file. You should
     * probably add some default build and run settings to the project so that
     * it can be build and run.
     */
    virtual bool restoreSettingsImpl(PersistentSettingsReader &reader);

private:
    QList<BuildConfiguration *> m_buildConfigurationValues;
    BuildConfiguration *m_activeBuildConfiguration;
    QList<RunConfiguration *> m_runConfigurations;
    RunConfiguration* m_activeRunConfiguration;
    EditorConfiguration *m_editorConfiguration;
};

} // namespace ProjectExplorer

#endif // PROJECT_H
