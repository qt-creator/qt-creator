/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef BUILDCONFIGURATION_H
#define BUILDCONFIGURATION_H

#include "projectexplorer_export.h"
#include "projectconfiguration.h"

#include <utils/environment.h>

#include <QStringList>

namespace Utils {
class AbstractMacroExpander;
}

namespace ProjectExplorer {

class BuildConfiguration;
class BuildConfigWidget;
class BuildStepList;
class Profile;
class Target;
class IOutputParser;

class PROJECTEXPLORER_EXPORT BuildConfiguration : public ProjectConfiguration
{
    Q_OBJECT

public:
    // ctors are protected
    virtual ~BuildConfiguration();

    virtual QString buildDirectory() const = 0;

    virtual BuildConfigWidget *createConfigWidget() = 0;

    // Maybe the BuildConfiguration is not the best place for the environment
    virtual Utils::Environment baseEnvironment() const;
    QString baseEnvironmentText() const;
    Utils::Environment environment() const;
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);
    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;
    bool useSystemEnvironment() const;
    void setUseSystemEnvironment(bool b);

    QList<Core::Id> knownStepLists() const;
    BuildStepList *stepList(Core::Id id) const;

    virtual QVariantMap toMap() const;

    // Creates a suitable outputparser for custom build steps
    // (based on the tool chain)
    // this is not great API
    // it's mainly so that custom build systems are better integrated
    // with the generic project manager
    virtual IOutputParser *createOutputParser() const = 0;

    Target *target() const;

    virtual bool isEnabled() const;
    virtual QString disabledReason() const;

    Utils::AbstractMacroExpander *macroExpander();

    enum BuildType {
        Unknown,
        Debug,
        Release
    };
    virtual BuildType buildType() const = 0;

signals:
    void environmentChanged();
    void buildDirectoryChanged();
    void enabledChanged();

protected:
    BuildConfiguration(Target *target, const Core::Id id);
    BuildConfiguration(Target *target, BuildConfiguration *source);

    void cloneSteps(BuildConfiguration *source);

    virtual bool fromMap(const QVariantMap &map);

private slots:
    void handleProfileUpdate(ProjectExplorer::Profile *p);

private:
    bool m_clearSystemEnvironment;
    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
    QList<BuildStepList *> m_stepLists;
    Utils::AbstractMacroExpander *m_macroExpander;
};

class PROJECTEXPLORER_EXPORT IBuildConfigurationFactory :
    public QObject
{
    Q_OBJECT

public:
    explicit IBuildConfigurationFactory(QObject *parent = 0);
    virtual ~IBuildConfigurationFactory();

    // used to show the list of possible additons to a target, returns a list of types
    virtual QList<Core::Id> availableCreationIds(const Target *parent) const = 0;
    // used to translate the types to names to display to the user
    virtual QString displayNameForId(const Core::Id id) const = 0;

    virtual bool canCreate(const Target *parent, const Core::Id id) const = 0;
    virtual BuildConfiguration *create(Target *parent, const Core::Id id, const QString &name = QString()) = 0;
    // used to recreate the runConfigurations when restoring settings
    virtual bool canRestore(const Target *parent, const QVariantMap &map) const = 0;
    virtual BuildConfiguration *restore(Target *parent, const QVariantMap &map) = 0;
    virtual bool canClone(const Target *parent, BuildConfiguration *product) const = 0;
    virtual BuildConfiguration *clone(Target *parent, BuildConfiguration *product) = 0;

    static IBuildConfigurationFactory *find(Target *parent, const QVariantMap &map);
    static IBuildConfigurationFactory *find(Target *parent);

signals:
    void availableCreationIdsChanged();
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::BuildConfiguration *)

#endif // BUILDCONFIGURATION_H
