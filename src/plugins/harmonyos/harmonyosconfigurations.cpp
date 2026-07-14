// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosconfigurations.h"

#include "harmonyosconstants.h"
#include "harmonyosqtversion.h"
#include "harmonyossdk.h"
#include "harmonyossettings.h"
#include "harmonyostoolchain.h"
#include "harmonyostr.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/sysrootkitaspect.h>
#include <projectexplorer/toolchainkitaspect.h>
#include <projectexplorer/toolchainmanager.h>

#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggerkitaspect.h>

#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtversionmanager.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace HarmonyOs::Internal {

void registerNewToolchains()
{
    const Toolchains existingToolchains = ToolchainManager::toolchains(
        Utils::equal(&Toolchain::typeId, Id(Constants::HARMONYOS_TOOLCHAIN_TYPEID)));
    ToolchainManager::registerToolchains(autodetectToolchains(existingToolchains));

    // Drop toolchains that no longer point at a valid SDK.
    const Toolchains invalidToolchains = ToolchainManager::toolchains([](const Toolchain *tc) {
        return tc->typeId() == Constants::HARMONYOS_TOOLCHAIN_TYPEID && !tc->isValid();
    });
    ToolchainManager::deregisterToolchains(invalidToolchains);
}

// Whether a Qt version targets HarmonyOS.
//
// This is normally decided by the version's type. As a workaround we also accept versions that
// the Qt online installer mislabels as Desktop but that report an OpenHarmony ABI (see
// QTCREATORBUG-34793). Remove the ABI fallback once the installer registers HarmonyOS Qt with
// QtVersion.Type=Qt4ProjectManager.QtVersion.HarmonyOS.
static bool isHarmonyOsQtVersion(const QtVersion *v)
{
    if (v->type() == Constants::HARMONYOS_QT_TYPE)
        return true;
    return Utils::anyOf(v->qtAbis(), [](const Abi &abi) {
        return abi.osFlavor() == Abi::OpenHarmonyLinuxFlavor;
    });
}

// The HarmonyOS ABI name (as used by the Qt installation directories and the ohos mkspec),
// used to disambiguate kits when several target architectures are installed.
static QString ohosAbiName(const Abi &abi)
{
    if (abi.architecture() == Abi::ArmArchitecture)
        return abi.wordWidth() == 64 ? QString("arm64-v8a") : QString("armeabi-v7a");
    if (abi.architecture() == Abi::X86Architecture && abi.wordWidth() == 64)
        return QString("x86_64");
    return abi.toString();
}

static QVariant findOrRegisterDebugger(const FilePath &lldb)
{
    if (lldb.isEmpty() || !lldb.exists())
        return {};

    const Debugger::DebuggerItem existing = Debugger::DebuggerItemManager::findByCommand(lldb);
    if (existing && existing.engineType() == Debugger::LldbEngineType
        && existing.detectionSource().isAutoDetected()) {
        return existing.id();
    }

    Debugger::DebuggerItem debugger;
    debugger.setCommand(lldb);
    debugger.setEngineType(Debugger::LldbEngineType);
    debugger.setUnexpandedDisplayName(Tr::tr("HarmonyOS LLDB"));
    debugger.setDetectionSource(DetectionSource::FromSystem);
    debugger.reinitializeFromFile();
    return Debugger::DebuggerItemManager::registerDebugger(debugger);
}

static bool matchKit(const ToolchainBundle &bundle, const Kit &kit)
{
    using namespace ProjectExplorer::Constants;
    for (const Id lang : {Id(C_LANGUAGE_ID), Id(CXX_LANGUAGE_ID)}) {
        const Toolchain * const tc = ToolchainKitAspect::toolchain(&kit, lang);
        if (!tc || tc->typeId() != Constants::HARMONYOS_TOOLCHAIN_TYPEID
            || tc->targetAbi() != bundle.targetAbi()) {
            return false;
        }
    }
    return true;
}

void updateAutomaticKitList()
{
    const QList<Kit *> existingKits = Utils::filtered(KitManager::kits(), [](Kit *k) {
        return !k->detectionSource().isSdkProvided()
               && RunDeviceTypeKitAspect::deviceTypeId(k) == Constants::HARMONYOS_DEVICE_TYPE;
    });

    const auto removeAutoKits = [&existingKits] {
        const QList<Kit *> autoKits = Utils::filtered(existingKits, [](Kit *k) {
            return k->detectionSource().id == Constants::harmonyOsConfigurationId;
        });
        KitManager::deregisterKits(autoKits);
    };

    if (!settings().automaticKitCreation()) {
        removeAutoKits();
        return;
    }

    // Group HarmonyOS Qt versions by their (single) ABI.
    QHash<Abi, QList<const QtVersion *>> qtVersionsForArch;
    const QtVersions qtVersions = QtVersionManager::versions(
        [](const QtVersion *v) { return isHarmonyOsQtVersion(v); });
    for (const QtVersion *qtVersion : qtVersions) {
        const Abis qtAbis = qtVersion->qtAbis();
        if (qtAbis.isEmpty())
            continue;
        qtVersionsForArch[qtAbis.first()].append(qtVersion);
    }

    const QList<ToolchainBundle> bundles = Utils::filtered(
        ToolchainBundle::collectBundles(
            ToolchainManager::toolchains([](const Toolchain *tc) {
                return tc->detectionSource().isAutoDetected()
                       && tc->typeId() == Constants::HARMONYOS_TOOLCHAIN_TYPEID;
            }),
            ToolchainBundle::HandleMissing::CreateAndRegister),
        [](const ToolchainBundle &b) { return b.isCompletelyValid(); });

    const QVariant debuggerId = findOrRegisterDebugger(
        Sdk::lldbCommand(settings().sdkLocation()));
    const FilePath sysroot = Sdk::sysrootPath(settings().sdkLocation());

    QList<Kit *> unhandledKits = existingKits;
    for (const ToolchainBundle &bundle : bundles) {
        const QString abiName = ohosAbiName(bundle.targetAbi());
        for (const QtVersion *qt : qtVersionsForArch.value(bundle.targetAbi())) {
            Kit *existingKit = Utils::findOrDefault(existingKits, [&](const Kit *b) {
                return qt == QtKitAspect::qtVersion(b) && matchKit(bundle, *b);
            });

            const auto initializeKit = [&bundle, qt, debuggerId, sysroot](Kit *k) {
                const auto source = qt->detectionSource().isAutoDetected()
                                        ? DetectionSource::FromSystem
                                        : DetectionSource::Manual;
                k->setDetectionSource({source, Constants::harmonyOsConfigurationId});
                RunDeviceTypeKitAspect::setDeviceTypeId(k, Constants::HARMONYOS_DEVICE_TYPE);
                ToolchainKitAspect::setBundle(k, bundle);
                QtKitAspect::setQtVersion(k, qt);
                Debugger::DebuggerKitAspect::setDebugger(k, debuggerId);
                if (!sysroot.isEmpty())
                    SysRootKitAspect::setSysRoot(k, sysroot);
                const auto desktopDevice = DeviceManager::defaultDesktopDevice();
                QTC_ASSERT(desktopDevice, return);
                BuildDeviceKitAspect::setDeviceId(k, desktopDevice->id());
                k->setSticky(QtKitAspect::id(), true);
                k->setSticky(RunDeviceTypeKitAspect::id(), true);
                k->setValueSilently(Constants::HARMONYOS_KIT_SDK,
                                    settings().sdkLocation().toSettings());
            };

            if (existingKit) {
                initializeKit(existingKit); // Update the existing kit with fresh data.
                unhandledKits.removeOne(existingKit);
            } else {
                KitManager::registerKit([initializeKit, abiName](Kit *k) {
                    k->setUnexpandedDisplayName(
                        Tr::tr("%1 for HarmonyOS %2")
                            .arg(QLatin1String("Qt %{Qt:Version}"), abiName));
                    initializeKit(k);
                });
            }
        }
    }

    // Remove auto-created kits that were not re-used.
    unhandledKits = Utils::filtered(unhandledKits, [](Kit *k) {
        return k->detectionSource().id == Constants::harmonyOsConfigurationId;
    });
    KitManager::deregisterKits(unhandledKits);
}

void applyConfig()
{
    registerNewToolchains();
    updateAutomaticKitList();
}

} // namespace HarmonyOs::Internal
