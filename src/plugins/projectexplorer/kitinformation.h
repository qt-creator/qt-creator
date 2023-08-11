// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abi.h"
#include "devicesupport/idevicefwd.h"
#include "kitmanager.h"
#include "kit.h"

#include <utils/environment.h>

#include <QVariant>

namespace ProjectExplorer {
class OutputTaskParser;
class ToolChain;

class KitAspect;

// --------------------------------------------------------------------------
// SysRootInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT SysRootKitAspect
{
public:
    static Utils::Id id();
    static Utils::FilePath sysRoot(const Kit *k);
    static void setSysRoot(Kit *k, const Utils::FilePath &v);
};

class PROJECTEXPLORER_EXPORT SysRootKitAspectFactory : public KitAspectFactory
{
public:
    SysRootKitAspectFactory();

    Tasks validate(const Kit *k) const override;
    KitAspect *createKitAspect(Kit *k) const override;
    ItemList toUserOutput(const Kit *k) const override;
    void addToMacroExpander(Kit *kit, Utils::MacroExpander *expander) const override;
};

// --------------------------------------------------------------------------
// ToolChainInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChainKitAspect
{
public:
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
};

class PROJECTEXPLORER_EXPORT ToolChainKitAspectFactory : public KitAspectFactory
{
public:
    ToolChainKitAspectFactory();

    Tasks validate(const Kit *k) const override;
    void upgrade(Kit *k) override;
    void fix(Kit *k) override;
    void setup(Kit *k) override;

    KitAspect *createKitAspect(Kit *k) const override;

    QString displayNamePostfix(const Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    void addToBuildEnvironment(const Kit *k, Utils::Environment &env) const override;
    void addToRunEnvironment(const Kit *, Utils::Environment &) const override {}

    void addToMacroExpander(Kit *kit, Utils::MacroExpander *expander) const override;
    QList<Utils::OutputLineParser *> createOutputParsers(const Kit *k) const override;
    QSet<Utils::Id> availableFeatures(const Kit *k) const override;

private:
    void kitsWereLoaded();
    void toolChainUpdated(ProjectExplorer::ToolChain *tc);
    void toolChainRemoved(ProjectExplorer::ToolChain *tc);
};

// --------------------------------------------------------------------------
// DeviceTypeInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT DeviceTypeKitAspect
{
public:
    static const Utils::Id id();
    static const Utils::Id deviceTypeId(const Kit *k);
    static void setDeviceTypeId(Kit *k, Utils::Id type);
};

class PROJECTEXPLORER_EXPORT DeviceTypeKitAspectFactory : public KitAspectFactory
{
public:
    DeviceTypeKitAspectFactory();

    void setup(Kit *k) override;
    Tasks validate(const Kit *k) const override;
    KitAspect *createKitAspect(Kit *k) const override;
    ItemList toUserOutput(const Kit *k) const override;

    QSet<Utils::Id> supportedPlatforms(const Kit *k) const override;
    QSet<Utils::Id> availableFeatures(const Kit *k) const override;
};

// --------------------------------------------------------------------------
// DeviceInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT DeviceKitAspect
{
public:
    static Utils::Id id();
    static IDeviceConstPtr device(const Kit *k);
    static Utils::Id deviceId(const Kit *k);
    static void setDevice(Kit *k, IDeviceConstPtr dev);
    static void setDeviceId(Kit *k, Utils::Id dataId);
    static Utils::FilePath deviceFilePath(const Kit *k, const QString &pathOnDevice);
};

class PROJECTEXPLORER_EXPORT DeviceKitAspectFactory : public KitAspectFactory
{
public:
    DeviceKitAspectFactory();

    Tasks validate(const Kit *k) const override;
    void fix(Kit *k) override;
    void setup(Kit *k) override;

    KitAspect *createKitAspect(Kit *k) const override;

    QString displayNamePostfix(const Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    void addToMacroExpander(ProjectExplorer::Kit *kit, Utils::MacroExpander *expander) const override;

private:
    QVariant defaultValue(const Kit *k) const;

    void kitsWereLoaded();
    void deviceUpdated(Utils::Id dataId);
    void devicesChanged();
    void kitUpdated(ProjectExplorer::Kit *k);
};

// --------------------------------------------------------------------------
// BuildDeviceInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT BuildDeviceKitAspect
{
public:
    static Utils::Id id();
    static IDeviceConstPtr device(const Kit *k);
    static Utils::Id deviceId(const Kit *k);
    static void setDevice(Kit *k, IDeviceConstPtr dev);
    static void setDeviceId(Kit *k, Utils::Id dataId);
};

class PROJECTEXPLORER_EXPORT BuildDeviceKitAspectFactory : public KitAspectFactory
{
public:
    BuildDeviceKitAspectFactory();

    void setup(Kit *k) override;
    Tasks validate(const Kit *k) const override;

    KitAspect *createKitAspect(Kit *k) const override;

    QString displayNamePostfix(const Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    void addToMacroExpander(ProjectExplorer::Kit *kit, Utils::MacroExpander *expander) const override;

private:
    void kitsWereLoaded();
    void deviceUpdated(Utils::Id dataId);
    void devicesChanged();
    void kitUpdated(ProjectExplorer::Kit *k);
};

// --------------------------------------------------------------------------
// EnvironmentKitAspect:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT EnvironmentKitAspect
{
public:
    static Utils::Id id();
    static Utils::EnvironmentItems environmentChanges(const Kit *k);
    static void setEnvironmentChanges(Kit *k, const Utils::EnvironmentItems &changes);
};

class PROJECTEXPLORER_EXPORT EnvironmentKitAspectFactory : public KitAspectFactory
{
public:
    EnvironmentKitAspectFactory();

    Tasks validate(const Kit *k) const override;
    void fix(Kit *k) override;

    void addToBuildEnvironment(const Kit *k, Utils::Environment &env) const override;
    void addToRunEnvironment(const Kit *, Utils::Environment &) const override;

    KitAspect *createKitAspect(Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;
};

} // namespace ProjectExplorer
