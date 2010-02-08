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

#ifndef TARGET_H
#define TARGET_H

#include "projectconfiguration.h"
#include "projectexplorer_export.h"

#include <QtCore/QObject>
#include <QtGui/QFileSystemModel>
#include <QtGui/QIcon>

namespace ProjectExplorer {

class RunConfiguration;
class Environment;
class BuildConfiguration;
class IBuildConfigurationFactory;
class IRunConfigurationFactory;
class Project;

class PROJECTEXPLORER_EXPORT Target : public ProjectConfiguration
{
    Q_OBJECT

public:
    virtual ~Target();

    virtual Project *project() const;

    // Build configuration
    void addBuildConfiguration(BuildConfiguration *configuration);
    void removeBuildConfiguration(BuildConfiguration *configuration);

    QList<BuildConfiguration *> buildConfigurations() const;
    virtual BuildConfiguration *activeBuildConfiguration() const;
    void setActiveBuildConfiguration(BuildConfiguration *configuration);

    virtual IBuildConfigurationFactory *buildConfigurationFactory() const = 0;

    // Running
    QList<RunConfiguration *> runConfigurations() const;
    void addRunConfiguration(RunConfiguration* runConfiguration);
    void removeRunConfiguration(RunConfiguration* runConfiguration);

    RunConfiguration* activeRunConfiguration() const;
    void setActiveRunConfiguration(RunConfiguration* runConfiguration);

    // Returns whether this target is actually available at he time
    // of the call. A target may become unavailable e.g. when a Qt version
    // is removed.
    //
    // Note: Enabled state is not saved!
    bool isEnabled() const;

    QIcon icon() const;
    void setIcon(QIcon icon);

    virtual QVariantMap toMap() const;

signals:
    void targetEnabled(bool);
    void iconChanged();

    // TODO clean up signal names
    // might be better to also have aboutToRemove signals
    void runConfigurationsEnabledStateChanged();

    void removedRunConfiguration(ProjectExplorer::RunConfiguration *);
    void addedRunConfiguration(ProjectExplorer::RunConfiguration *);
    void activeRunConfigurationChanged(ProjectExplorer::RunConfiguration *);

    void removedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void addedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration *);

    /// convenience signal, emitted if either the active buildconfiguration emits
    /// environmentChanged() or if the active build configuration changes
    void environmentChanged();

protected:
    Target(Project *parent, const QString &id);

    void setEnabled(bool);

    virtual bool fromMap(const QVariantMap &map);

private slots:
    void changeEnvironment();

private:
    Project *m_project;
    bool m_isEnabled;
    QIcon m_icon;

    QList<BuildConfiguration *> m_buildConfigurations;
    BuildConfiguration *m_activeBuildConfiguration;
    QList<RunConfiguration *> m_runConfigurations;
    RunConfiguration* m_activeRunConfiguration;
};

class PROJECTEXPLORER_EXPORT ITargetFactory :
    public QObject
{
    Q_OBJECT

public:
    explicit ITargetFactory(QObject *parent = 0);
    virtual ~ITargetFactory();

    // used to show the list of possible additons to a target, returns a list of types
    virtual QStringList availableCreationIds(Project *parent) const = 0;
    // used to translate the types to names to display to the user
    virtual QString displayNameForId(const QString &id) const = 0;

    virtual bool canCreate(Project *parent, const QString &id) const = 0;
    virtual Target *create(Project *parent, const QString &id) = 0;
    virtual bool canRestore(Project *parent, const QVariantMap &map) const = 0;
    virtual Target *restore(Project *parent, const QVariantMap &map) = 0;

signals:
    void availableCreationIdsChanged();
};

} // namespace ProjectExplorer

#endif // TARGET_H
