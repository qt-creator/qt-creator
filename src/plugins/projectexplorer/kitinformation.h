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

#include <utils/environment.h>

#include <QVariant>

namespace ProjectExplorer {
class OutputTaskParser;
class ToolChain;

class KitAspectWidget;

// --------------------------------------------------------------------------
// SysRootInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT SysRootKitAspect : public KitAspect
{
    Q_OBJECT

public:
    SysRootKitAspect();

    Tasks validate(const Kit *k) const override;
    KitAspectWidget *createConfigWidget(Kit *k) const override;
    ItemList toUserOutput(const Kit *k) const override;
    void addToMacroExpander(Kit *kit, Utils::MacroExpander *expander) const override;

    static Utils::Id id();
    static Utils::FilePath sysRoot(const Kit *k);
    static void setSysRoot(Kit *k, const Utils::FilePath &v);
};

// --------------------------------------------------------------------------
// ToolChainInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChainKitAspect : public KitAspect
{
    Q_OBJECT

public:
    ToolChainKitAspect();

    Tasks validate(const Kit *k) const override;
    void upgrade(Kit *k) override;
    void fix(Kit *k) override;
    void setup(Kit *k) override;

    KitAspectWidget *createConfigWidget(Kit *k) const override;

    QString displayNamePostfix(const Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    void addToEnvironment(const Kit *k, Utils::Environment &env) const override;
    void addToMacroExpander(Kit *kit, Utils::MacroExpander *expander) const override;
    QList<Utils::OutputLineParser *> createOutputParsers(const Kit *k) const override;
    QSet<Utils::Id> availableFeatures(const Kit *k) const override;

    static Utils::Id id();
    static QByteArray toolChainId(const Kit *k, Utils::Id language);
    static ToolChain *toolChain(const Kit *k, Utils::Id language);
    static ToolChain *cToolChain(const Kit *k);
    static ToolChain *cxxToolChain(const Kit *k);
    static QList<ToolChain *> toolChains(const Kit *k);
    static void setToolChain(Kit *k, ToolChain *tc);
    static void setAllToolChainsToMatch(Kit *k, ToolChain *tc);
    static void clearToolChain(Kit *k, Utils::Id language);
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

class PROJECTEXPLORER_EXPORT DeviceTypeKitAspect : public KitAspect
{
    Q_OBJECT

public:
    DeviceTypeKitAspect();

    void setup(Kit *k) override;
    Tasks validate(const Kit *k) const override;
    KitAspectWidget *createConfigWidget(Kit *k) const override;
    ItemList toUserOutput(const Kit *k) const override;

    static const Utils::Id id();
    static const Utils::Id deviceTypeId(const Kit *k);
    static void setDeviceTypeId(Kit *k, Utils::Id type);

    QSet<Utils::Id> supportedPlatforms(const Kit *k) const override;
    QSet<Utils::Id> availableFeatures(const Kit *k) const override;
};

// --------------------------------------------------------------------------
// DeviceInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT DeviceKitAspect : public KitAspect
{
    Q_OBJECT

public:
    DeviceKitAspect();

    Tasks validate(const Kit *k) const override;
    void fix(Kit *k) override;
    void setup(Kit *k) override;

    KitAspectWidget *createConfigWidget(Kit *k) const override;

    QString displayNamePostfix(const Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    void addToMacroExpander(ProjectExplorer::Kit *kit, Utils::MacroExpander *expander) const override;

    static Utils::Id id();
    static IDevice::ConstPtr device(const Kit *k);
    static Utils::Id deviceId(const Kit *k);
    static void setDevice(Kit *k, IDevice::ConstPtr dev);
    static void setDeviceId(Kit *k, Utils::Id dataId);

private:
    QVariant defaultValue(const Kit *k) const;

    void kitsWereLoaded();
    void deviceUpdated(Utils::Id dataId);
    void devicesChanged();
    void kitUpdated(ProjectExplorer::Kit *k);
};

// --------------------------------------------------------------------------
// EnvironmentKitAspect:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT EnvironmentKitAspect : public KitAspect
{
    Q_OBJECT

public:
    EnvironmentKitAspect();

    Tasks validate(const Kit *k) const override;
    void fix(Kit *k) override;

    void addToEnvironment(const Kit *k, Utils::Environment &env) const override;
    KitAspectWidget *createConfigWidget(Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    static Utils::Id id();
    static Utils::EnvironmentItems environmentChanges(const Kit *k);
    static void setEnvironmentChanges(Kit *k, const Utils::EnvironmentItems &changes);
};

} // namespace ProjectExplorer
