/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "mcutarget.h"
#include "mcupackage.h"
#include "mcukitmanager.h"

#include "mcusupportplugin.h"

#include <baremetal/baremetalconstants.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport::Internal {

static ToolChain *msvcToolChain(Id language)
{
    ToolChain *toolChain = ToolChainManager::toolChain([language](const ToolChain *t) {
        const Abi abi = t->targetAbi();
        // TODO: Should Abi::WindowsMsvc2022Flavor be added too?
        return (abi.osFlavor() == Abi::WindowsMsvc2017Flavor
                || abi.osFlavor() == Abi::WindowsMsvc2019Flavor)
               && abi.architecture() == Abi::X86Architecture && abi.wordWidth() == 64
               && t->language() == language;
    });
    return toolChain;
}

static ToolChain *gccToolChain(Id language)
{
    ToolChain *toolChain = ToolChainManager::toolChain([language](const ToolChain *t) {
        const Abi abi = t->targetAbi();
        return abi.os() != Abi::WindowsOS && abi.architecture() == Abi::X86Architecture
               && abi.wordWidth() == 64 && t->language() == language;
    });
    return toolChain;
}

static ToolChain *armGccToolChain(const FilePath &path, Id language)
{
    ToolChain *toolChain = ToolChainManager::toolChain([&path, language](const ToolChain *t) {
        return t->compilerCommand() == path && t->language() == language;
    });
    if (!toolChain) {
        ToolChainFactory *gccFactory
            = Utils::findOrDefault(ToolChainFactory::allToolChainFactories(),
                                   [](ToolChainFactory *f) {
                                       return f->supportedToolChainType()
                                              == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID;
                                   });
        if (gccFactory) {
            const QList<ToolChain *> detected = gccFactory->detectForImport({path, language});
            if (!detected.isEmpty()) {
                toolChain = detected.first();
                toolChain->setDetection(ToolChain::ManualDetection);
                toolChain->setDisplayName("Arm GCC");
                ToolChainManager::registerToolChain(toolChain);
            }
        }
    }

    return toolChain;
}

static ToolChain *iarToolChain(const FilePath &path, Id language)
{
    ToolChain *toolChain = ToolChainManager::toolChain([language](const ToolChain *t) {
        return t->typeId() == BareMetal::Constants::IAREW_TOOLCHAIN_TYPEID
               && t->language() == language;
    });
    if (!toolChain) {
        ToolChainFactory *iarFactory
            = Utils::findOrDefault(ToolChainFactory::allToolChainFactories(),
                                   [](ToolChainFactory *f) {
                                       return f->supportedToolChainType()
                                              == BareMetal::Constants::IAREW_TOOLCHAIN_TYPEID;
                                   });
        if (iarFactory) {
            Toolchains detected = iarFactory->autoDetect(ToolchainDetector({}, {}));
            if (detected.isEmpty())
                detected = iarFactory->detectForImport({path, language});
            for (auto tc : detected) {
                if (tc->language() == language) {
                    toolChain = tc;
                    toolChain->setDetection(ToolChain::ManualDetection);
                    toolChain->setDisplayName("IAREW");
                    ToolChainManager::registerToolChain(toolChain);
                }
            }
        }
    }

    return toolChain;
}

ToolChain *McuToolChainPackage::toolChain(Id language) const
{
    switch (m_type) {
    case Type::MSVC:
        return msvcToolChain(language);
    case Type::GCC:
        return gccToolChain(language);
    case Type::IAR: {
        const FilePath compiler = path().pathAppended("/bin/iccarm").withExecutableSuffix();
        return iarToolChain(compiler, language);
    }
    case Type::ArmGcc:
    case Type::KEIL:
    case Type::GHS:
    case Type::GHSArm:
    case Type::Unsupported: {
        const QLatin1String compilerName(
            language == ProjectExplorer::Constants::C_LANGUAGE_ID ? "gcc" : "g++");
        const QString comp = QLatin1String(m_type == Type::ArmGcc ? "/bin/arm-none-eabi-%1"
                                                                  : "/bar/foo-keil-%1")
                                 .arg(compilerName);
        const FilePath compiler = path().pathAppended(comp).withExecutableSuffix();

        return armGccToolChain(compiler, language);
    }
    default:
        Q_UNREACHABLE();
    }
}

QString McuToolChainPackage::toolChainName() const
{
    switch (m_type) {
    case Type::ArmGcc:
        return QLatin1String("armgcc");
    case Type::IAR:
        return QLatin1String("iar");
    case Type::KEIL:
        return QLatin1String("keil");
    case Type::GHS:
        return QLatin1String("ghs");
    case Type::GHSArm:
        return QLatin1String("ghs-arm");
    default:
        return QLatin1String("unsupported");
    }
}

QString McuToolChainPackage::cmakeToolChainFileName() const
{
    return toolChainName() + QLatin1String(".cmake");
}

QVariant McuToolChainPackage::debuggerId() const
{
    using namespace Debugger;

    QString sub, displayName;
    DebuggerEngineType engineType;

    switch (m_type) {
    case Type::ArmGcc: {
        sub = QString::fromLatin1("bin/arm-none-eabi-gdb-py");
        displayName = McuPackage::tr("Arm GDB at %1");
        engineType = Debugger::GdbEngineType;
        break;
    }
    case Type::IAR: {
        sub = QString::fromLatin1("../common/bin/CSpyBat");
        displayName = QLatin1String("CSpy");
        engineType = Debugger::NoEngineType; // support for IAR missing
        break;
    }
    case Type::KEIL: {
        sub = QString::fromLatin1("UV4/UV4");
        displayName = QLatin1String("KEIL uVision Debugger");
        engineType = Debugger::UvscEngineType;
        break;
    }
    default:
        return QVariant();
    }

    const FilePath command = path().pathAppended(sub).withExecutableSuffix();
    if (const DebuggerItem *debugger = DebuggerItemManager::findByCommand(command)) {
        return debugger->id();
    }

    DebuggerItem newDebugger;
    newDebugger.setCommand(command);
    newDebugger.setUnexpandedDisplayName(displayName.arg(command.toUserOutput()));
    newDebugger.setEngineType(engineType);
    return DebuggerItemManager::registerDebugger(newDebugger);
}

McuTarget::McuTarget(const QVersionNumber &qulVersion,
                     const Platform &platform,
                     OS os,
                     const QVector<McuAbstractPackage *> &packages,
                     const McuToolChainPackage *toolChainPackage,
                     int colorDepth)
    : m_qulVersion(qulVersion)
    , m_platform(platform)
    , m_os(os)
    , m_packages(packages)
    , m_toolChainPackage(toolChainPackage)
    , m_colorDepth(colorDepth)
{}

const QVector<McuAbstractPackage *> &McuTarget::packages() const
{
    return m_packages;
}

const McuToolChainPackage *McuTarget::toolChainPackage() const
{
    return m_toolChainPackage;
}

McuTarget::OS McuTarget::os() const
{
    return m_os;
}

const McuTarget::Platform &McuTarget::platform() const
{
    return m_platform;
}

bool McuTarget::isValid() const
{
    return Utils::allOf(packages(), [](McuAbstractPackage *package) {
        package->updateStatus();
        return package->validStatus();
    });
}

void McuTarget::printPackageProblems() const
{
    for (auto package : packages()) {
        package->updateStatus();
        if (!package->validStatus())
            printMessage(tr("Error creating kit for target %1, package %2: %3")
                             .arg(McuKitManager::kitName(this),
                                  package->label(),
                                  package->statusText()),
                         true);
        if (package->status() == McuAbstractPackage::Status::ValidPackageMismatchedVersion)
            printMessage(tr("Warning creating kit for target %1, package %2: %3")
                             .arg(McuKitManager::kitName(this),
                                  package->label(),
                                  package->statusText()),
                         false);
    }
}

const QVersionNumber &McuTarget::qulVersion() const
{
    return m_qulVersion;
}

int McuTarget::colorDepth() const
{
    return m_colorDepth;
}

} // namespace McuSupport::Internal
