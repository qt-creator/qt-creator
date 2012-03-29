/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef TARGET_H
#define TARGET_H

#include "projectconfiguration.h"
#include "projectexplorer_export.h"

QT_FORWARD_DECLARE_CLASS(QIcon)

namespace Utils {
class Environment;
}

namespace ProjectExplorer {
class RunConfiguration;
class ToolChain;
class BuildConfiguration;
class DeployConfiguration;
class IBuildConfigurationFactory;
class DeployConfigurationFactory;
class IRunConfigurationFactory;
class Project;
class BuildConfigWidget;

class TargetPrivate;

class PROJECTEXPLORER_EXPORT Target : public ProjectConfiguration
{
    Q_OBJECT

public:
    virtual ~Target();

    virtual BuildConfigWidget *createConfigWidget() = 0;
    Project *project() const;

    // Build configuration
    void addBuildConfiguration(BuildConfiguration *configuration);
    bool removeBuildConfiguration(BuildConfiguration *configuration);

    QList<BuildConfiguration *> buildConfigurations() const;
    BuildConfiguration *activeBuildConfiguration() const;
    void setActiveBuildConfiguration(BuildConfiguration *configuration);

    virtual IBuildConfigurationFactory *buildConfigurationFactory() const = 0;

    // DeployConfiguration
    void addDeployConfiguration(DeployConfiguration *dc);
    bool removeDeployConfiguration(DeployConfiguration *dc);

    QList<DeployConfiguration *> deployConfigurations() const;
    DeployConfiguration *activeDeployConfiguration() const;
    void setActiveDeployConfiguration(DeployConfiguration *configuration);

    QStringList availableDeployConfigurationIds();
    QString displayNameForDeployConfigurationId(const QString &id);
    DeployConfiguration *createDeployConfiguration(const QString &id);

    // Running
    QList<RunConfiguration *> runConfigurations() const;
    void addRunConfiguration(RunConfiguration *runConfiguration);
    void removeRunConfiguration(RunConfiguration *runConfiguration);

    RunConfiguration *activeRunConfiguration() const;
    void setActiveRunConfiguration(RunConfiguration *runConfiguration);

    // Returns whether this target is actually available at he time
    // of the call. A target may become unavailable e.g. when a Qt version
    // is removed.
    //
    // Note: Enabled state is not saved!
    bool isEnabled() const;

    QIcon icon() const;
    void setIcon(QIcon icon);
    QIcon overlayIcon() const;
    void setOverlayIcon(QIcon icon);
    QString toolTip() const;
    void setToolTip(const QString &text);

    virtual QList<ToolChain *> possibleToolChains(BuildConfiguration *) const;
    virtual ToolChain *preferredToolChain(BuildConfiguration *) const;

    virtual QVariantMap toMap() const;

signals:
    void targetEnabled(bool);
    void iconChanged();
    void overlayIconChanged();
    void toolTipChanged();

    // TODO clean up signal names
    // might be better to also have aboutToRemove signals
    void removedRunConfiguration(ProjectExplorer::RunConfiguration *);
    void addedRunConfiguration(ProjectExplorer::RunConfiguration *);
    void activeRunConfigurationChanged(ProjectExplorer::RunConfiguration *);

    void removedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void addedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration *);

    void removedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void addedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration *dc);

    /// convenience signal, emitted if either the active buildconfiguration emits
    /// environmentChanged() or if the active build configuration changes
    void environmentChanged();

    /// convenience signal, emitted if either the active configuration emits
    /// enabledChanged() or if the active build configuration changes
    void buildConfigurationEnabledChanged();
    void deployConfigurationEnabledChanged();
    void runConfigurationEnabledChanged();

protected:
    Target(Project *parent, const QString &id);

    void setEnabled(bool);

    virtual bool fromMap(const QVariantMap &map);

private slots:
    void changeEnvironment();
    void changeBuildConfigurationEnabled();
    void changeDeployConfigurationEnabled();
    void changeRunConfigurationEnabled();

private:
    TargetPrivate *d;
};

class PROJECTEXPLORER_EXPORT ITargetFactory :
    public QObject
{
    Q_OBJECT

public:
    explicit ITargetFactory(QObject *parent = 0);

    virtual QStringList supportedTargetIds() const = 0;
    virtual bool supportsTargetId(const QString &id) const = 0;

    // used to translate the types to names to display to the user
    virtual QString displayNameForId(const QString &id) const = 0;

    virtual bool canCreate(Project *parent, const QString &id) const = 0;
    virtual Target *create(Project *parent, const QString &id) = 0;
    virtual bool canRestore(Project *parent, const QVariantMap &map) const = 0;
    virtual Target *restore(Project *parent, const QVariantMap &map) = 0;

signals:
    void canCreateTargetIdsChanged();
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Target *)

#endif // TARGET_H
