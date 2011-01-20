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

#ifndef BUILDCONFIGURATION_H
#define BUILDCONFIGURATION_H

#include "projectexplorer_export.h"
#include "projectconfiguration.h"

#include <utils/stringutils.h>
#include <utils/environment.h>

#include <QtCore/QString>
#include <QtCore/QStringList>



namespace ProjectExplorer {

class BuildConfiguration;
class BuildStepList;
class Target;
class IOutputParser;

class BuildConfigMacroExpander : public Utils::AbstractQtcMacroExpander {
public:
    BuildConfigMacroExpander(BuildConfiguration *bc) : m_bc(bc) {}
    virtual bool resolveMacro(const QString &name, QString *ret);
private:
    BuildConfiguration *m_bc;
};

class PROJECTEXPLORER_EXPORT BuildConfiguration : public ProjectConfiguration
{
    Q_OBJECT

public:
    // ctors are protected
    virtual ~BuildConfiguration();

    virtual QString buildDirectory() const = 0;

    // TODO: Maybe the BuildConfiguration is not the best place for the environment
    virtual Utils::Environment baseEnvironment() const;
    QString baseEnvironmentText() const;
    Utils::Environment environment() const;
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);
    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;
    bool useSystemEnvironment() const;
    void setUseSystemEnvironment(bool b);

    QStringList knownStepLists() const;
    BuildStepList *stepList(const QString &id) const;

    virtual QVariantMap toMap() const;

    // Creates a suitable outputparser for custom build steps
    // (based on the toolchain)
    // TODO this is not great API
    // it's mainly so that custom build systems are better integrated
    // with the generic project manager
    virtual IOutputParser *createOutputParser() const = 0;

    Target *target() const;

    virtual bool isEnabled() const;

    Utils::AbstractMacroExpander *macroExpander() { return &m_macroExpander; }

signals:
    void environmentChanged();
    void buildDirectoryChanged();
    void enabledChanged();

protected:
    BuildConfiguration(Target *target, const QString &id);
    BuildConfiguration(Target *target, BuildConfiguration *source);

    void cloneSteps(BuildConfiguration *source);

    virtual bool fromMap(const QVariantMap &map);

private:
    bool m_clearSystemEnvironment;
    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
    QList<BuildStepList *> m_stepLists;
    BuildConfigMacroExpander m_macroExpander;
};

class PROJECTEXPLORER_EXPORT IBuildConfigurationFactory :
    public QObject
{
    Q_OBJECT

public:
    explicit IBuildConfigurationFactory(QObject *parent = 0);
    virtual ~IBuildConfigurationFactory();

    // used to show the list of possible additons to a target, returns a list of types
    virtual QStringList availableCreationIds(Target *parent) const = 0;
    // used to translate the types to names to display to the user
    virtual QString displayNameForId(const QString &id) const = 0;

    virtual bool canCreate(Target *parent, const QString &id) const = 0;
    virtual BuildConfiguration *create(Target *parent, const QString &id) = 0;
    // used to recreate the runConfigurations when restoring settings
    virtual bool canRestore(Target *parent, const QVariantMap &map) const = 0;
    virtual BuildConfiguration *restore(Target *parent, const QVariantMap &map) = 0;
    virtual bool canClone(Target *parent, BuildConfiguration *product) const = 0;
    virtual BuildConfiguration *clone(Target *parent, BuildConfiguration *product) = 0;

signals:
    void availableCreationIdsChanged();
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::BuildConfiguration *)

#endif // BUILDCONFIGURATION_H
