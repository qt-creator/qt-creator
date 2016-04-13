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
    ~DeployConfiguration() override;

    BuildStepList *stepList() const;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

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

    BuildStepList *m_stepList = nullptr;
};

class PROJECTEXPLORER_EXPORT DefaultDeployConfiguration : public DeployConfiguration
{
    Q_OBJECT
    friend class DefaultDeployConfigurationFactory; // for the ctors

protected:
    DefaultDeployConfiguration(Target *target, Core::Id id);
    DefaultDeployConfiguration(Target *target, DeployConfiguration *source);
};

class PROJECTEXPLORER_EXPORT DeployConfigurationFactory : public QObject
{
    Q_OBJECT

public:
    explicit DeployConfigurationFactory(QObject *parent = nullptr);

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
    QList<Core::Id> availableCreationIds(Target *parent) const override;
    // used to translate the types to names to display to the user
    QString displayNameForId(Core::Id id) const override;
    bool canCreate(Target *parent, Core::Id id) const override;
    DeployConfiguration *create(Target *parent, Core::Id id) override;
    bool canRestore(Target *parent, const QVariantMap &map) const override;
    DeployConfiguration *restore(Target *parent, const QVariantMap &map) override;
    bool canClone(Target *parent, DeployConfiguration *product) const override;
    DeployConfiguration *clone(Target *parent, DeployConfiguration *product) override;
private:
    bool canHandle(Target *parent) const;
};

} // namespace ProjectExplorer
