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

#include "abi.h"
#include "devicesupport/idevice.h"
#include "kitmanager.h"
#include "kit.h"
#include "toolchain.h"

#include <utils/environment.h>

#include <QVariant>

namespace ProjectExplorer {

class KitConfigWidget;

// --------------------------------------------------------------------------
// SysRootInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT SysRootKitInformation : public KitInformation
{
    Q_OBJECT

public:
    SysRootKitInformation();

    QVariant defaultValue(const Kit *k) const override;

    QList<Task> validate(const Kit *k) const override;

    KitConfigWidget *createConfigWidget(Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;
    void addToMacroExpander(Kit *kit, Utils::MacroExpander *expander) const override;

    static Core::Id id();
    static bool hasSysRoot(const Kit *k);
    static Utils::FileName sysRoot(const Kit *k);
    static void setSysRoot(Kit *k, const Utils::FileName &v);
};

// --------------------------------------------------------------------------
// ToolChainInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChainKitInformation : public KitInformation
{
    Q_OBJECT

public:
    ToolChainKitInformation();

    QVariant defaultValue(const Kit *k) const override;

    QList<Task> validate(const Kit *k) const override;
    void upgrade(Kit *k) override;
    void fix(Kit *k) override;
    void setup(Kit *k) override;

    KitConfigWidget *createConfigWidget(Kit *k) const override;

    QString displayNamePostfix(const Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    void addToEnvironment(const Kit *k, Utils::Environment &env) const override;
    void addToMacroExpander(Kit *kit, Utils::MacroExpander *expander) const override;
    IOutputParser *createOutputParser(const Kit *k) const override;
    QSet<Core::Id> availableFeatures(const Kit *k) const override;

    static Core::Id id();
    static ToolChain *toolChain(const Kit *k, Core::Id language);
    static QList<ToolChain *> toolChains(const Kit *k);
    static void setToolChain(Kit *k, ToolChain *tc);
    static void clearToolChain(Kit *k, Core::Id language);
    static Abi targetAbi(const Kit *k);

    static QString msgNoToolChainInTarget();

private:
    void kitsWereLoaded();
    void toolChainUpdated(ProjectExplorer::ToolChain *tc);
    void toolChainRemoved(ProjectExplorer::ToolChain *tc);
};

// --------------------------------------------------------------------------
// DeviceTypeInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT DeviceTypeKitInformation : public KitInformation
{
    Q_OBJECT

public:
    DeviceTypeKitInformation();

    QVariant defaultValue(const Kit *k) const override;

    QList<Task> validate(const Kit *k) const override;

    KitConfigWidget *createConfigWidget(Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    static const Core::Id id();
    static const Core::Id deviceTypeId(const Kit *k);
    static void setDeviceTypeId(Kit *k, Core::Id type);

    static Kit::Predicate deviceTypePredicate(Core::Id type);

    QSet<Core::Id> supportedPlatforms(const Kit *k) const override;
    QSet<Core::Id> availableFeatures(const Kit *k) const override;
};

// --------------------------------------------------------------------------
// DeviceInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT DeviceKitInformation : public KitInformation
{
    Q_OBJECT

public:
    DeviceKitInformation();

    QVariant defaultValue(const Kit *k) const override;

    QList<Task> validate(const Kit *k) const override;
    void fix(Kit *k) override;
    void setup(Kit *k) override;

    KitConfigWidget *createConfigWidget(Kit *k) const override;

    QString displayNamePostfix(const Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    virtual void addToMacroExpander(ProjectExplorer::Kit *kit, Utils::MacroExpander *expander) const override;

    static Core::Id id();
    static IDevice::ConstPtr device(const Kit *k);
    static Core::Id deviceId(const Kit *k);
    static void setDevice(Kit *k, IDevice::ConstPtr dev);
    static void setDeviceId(Kit *k, Core::Id dataId);

private:
    void kitsWereLoaded();
    void deviceUpdated(Core::Id dataId);
    void devicesChanged();
    void kitUpdated(ProjectExplorer::Kit *k);
};

// --------------------------------------------------------------------------
// EnvironmentKitInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT EnvironmentKitInformation : public KitInformation
{
    Q_OBJECT

public:
    EnvironmentKitInformation();

    QVariant defaultValue(const Kit *k) const override;

    QList<Task> validate(const Kit *k) const override;
    void fix(Kit *k) override;

    void addToEnvironment(const Kit *k, Utils::Environment &env) const override;
    KitConfigWidget *createConfigWidget(Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    static Core::Id id();
    static QList<Utils::EnvironmentItem> environmentChanges(const Kit *k);
    static void setEnvironmentChanges(Kit *k, const QList<Utils::EnvironmentItem> &changes);
};

} // namespace ProjectExplorer
