/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PROJECTEXPLORER_DEPLOYCONFIGURATION_H
#define PROJECTEXPLORER_DEPLOYCONFIGURATION_H

#include "projectexplorer_export.h"

#include "projectconfiguration.h"

#include <QString>

QT_FORWARD_DECLARE_CLASS(QStringList)

namespace ProjectExplorer {

class BuildStepList;
class Target;
class DeployConfigurationFactory;
class NamedWidget;

class PROJECTEXPLORER_EXPORT DeployConfiguration : public ProjectConfiguration
{
    Q_OBJECT

public:
    // ctors are protected
    virtual ~DeployConfiguration();

    BuildStepList *stepList() const;

    virtual bool fromMap(const QVariantMap &map);
    virtual QVariantMap toMap() const;

    virtual NamedWidget *createConfigWidget();

    virtual bool isEnabled() const;
    virtual QString disabledReason() const;

    Target *target() const;

signals:
    void enabledChanged();

protected:
    DeployConfiguration(Target *target, Core::Id id);
    DeployConfiguration(Target *target, DeployConfiguration *source);

    void cloneSteps(DeployConfiguration *source);

private:
    void ctor();

    BuildStepList *m_stepList;
};

class PROJECTEXPLORER_EXPORT DefaultDeployConfiguration : public DeployConfiguration
{
    Q_OBJECT
    friend class DefaultDeployConfigurationFactory; // for the ctors
protected:
    DefaultDeployConfiguration(Target *target, Core::Id id);
    DefaultDeployConfiguration(Target *target, DeployConfiguration *source);
};

class PROJECTEXPLORER_EXPORT DeployConfigurationFactory :
    public QObject
{
    Q_OBJECT

public:
    explicit DeployConfigurationFactory(QObject *parent = 0);
    virtual ~DeployConfigurationFactory();

    // used to show the list of possible additons to a target, returns a list of types
    virtual QList<Core::Id> availableCreationIds(Target *parent) const = 0;
    // used to translate the types to names to display to the user
    virtual QString displayNameForId(Core::Id id) const = 0;

    virtual bool canCreate(Target *parent, Core::Id id) const = 0;
    virtual DeployConfiguration *create(Target *parent, Core::Id id) = 0;
    // used to recreate the runConfigurations when restoring settings
    virtual bool canRestore(Target *parent, const QVariantMap &map) const = 0;
    virtual DeployConfiguration *restore(Target *parent, const QVariantMap &map) = 0;
    virtual bool canClone(Target *parent, DeployConfiguration *product) const = 0;
    virtual DeployConfiguration *clone(Target *parent, DeployConfiguration *product) = 0;

    static DeployConfigurationFactory *find(Target *parent, const QVariantMap &map);
    static QList<DeployConfigurationFactory *> find(Target *parent);
    static DeployConfigurationFactory *find(Target *parent, DeployConfiguration *dc);

signals:
    void availableCreationIdsChanged();
};

class DefaultDeployConfigurationFactory : public DeployConfigurationFactory
{
public:
    QList<Core::Id> availableCreationIds(Target *parent) const;
    // used to translate the types to names to display to the user
    QString displayNameForId(Core::Id id) const;
    bool canCreate(Target *parent, Core::Id id) const;
    DeployConfiguration *create(Target *parent, Core::Id id);
    bool canRestore(Target *parent, const QVariantMap &map) const;
    DeployConfiguration *restore(Target *parent, const QVariantMap &map);
    bool canClone(Target *parent, DeployConfiguration *product) const;
    DeployConfiguration *clone(Target *parent, DeployConfiguration *product);
private:
    bool canHandle(Target *parent) const;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::DeployConfiguration *)

#endif // PROJECTEXPLORER_DEPLOYCONFIGURATION_H
