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

#include "debuggerkitinformation.h"

#include "debuggeritemmanager.h"
#include "debuggeritem.h"
#include "debuggerkitconfigwidget.h"

#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <utility>

using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger {

// --------------------------------------------------------------------------
// DebuggerKitInformation
// --------------------------------------------------------------------------

DebuggerKitInformation::DebuggerKitInformation()
{
    setObjectName(QLatin1String("DebuggerKitInformation"));
    setId(DebuggerKitInformation::id());
    setPriority(28000);
}

QVariant DebuggerKitInformation::defaultValue(const Kit *k) const
{
    const Abi toolChainAbi = ToolChainKitInformation::targetAbi(k);
    foreach (const DebuggerItem &item, DebuggerItemManager::debuggers()) {
        foreach (const Abi targetAbi, item.abis()) {
            if (targetAbi.isCompatibleWith(toolChainAbi))
                return item.id();
        }
    }

    return QVariant();
}

void DebuggerKitInformation::setup(Kit *k)
{
    // This can be anything (Id, binary path, "auto")
    // With 3.0 we have:
    // <value type="QString" key="Debugger.Information">{75ecf347-f221-44c3-b613-ea1d29929cd4}</value>
    // Before we had:
    // <valuemap type="QVariantMap" key="Debugger.Information">
    //    <value type="QString" key="Binary">/data/dev/debugger/gdb-git/gdb/gdb</value>
    //    <value type="int" key="EngineType">1</value>
    //  </valuemap>
    // Or for force auto-detected CDB
    // <valuemap type="QVariantMap" key="Debugger.Information">
    //    <value type="QString" key="Binary">auto</value>
    //    <value type="int" key="EngineType">4</value>
    //  </valuemap>
    const QVariant rawId = k->value(DebuggerKitInformation::id());

    const Abi tcAbi = ToolChainKitInformation::targetAbi(k);

    // Get the best of the available debugger matching the kit's toolchain.
    // The general idea is to find an item that exactly matches what
    // is stored in the kit information, but also accept item based
    // on toolchain matching as fallback with a lower priority.

    DebuggerItem bestItem;
    DebuggerItem::MatchLevel bestLevel = DebuggerItem::DoesNotMatch;

    foreach (const DebuggerItem &item, DebuggerItemManager::debuggers()) {
        DebuggerItem::MatchLevel level = DebuggerItem::DoesNotMatch;

        if (rawId.isNull()) {
            // Initial setup of a kit.
            level = item.matchTarget(tcAbi);
        } else if (rawId.type() == QVariant::String) {
            // New structure.
            if (item.id() == rawId) {
                // Detected by ID.
                level = DebuggerItem::MatchesPerfectly;
            } else {
                // This item does not match by ID, and is an unlikely candidate.
                // However, consider using it as fallback if the tool chain fits.
                level = std::min(item.matchTarget(tcAbi), DebuggerItem::MatchesSomewhat);
            }
        } else {
            // Old structure.
            const QMap<QString, QVariant> map = rawId.toMap();
            QString binary = map.value(QLatin1String("Binary")).toString();
            if (binary == QLatin1String("auto")) {
                // This is close to the "new kit" case, except that we know
                // an engine type.
                DebuggerEngineType autoEngine = DebuggerEngineType(map.value(QLatin1String("EngineType")).toInt());
                if (item.engineType() == autoEngine) {
                    // Use item if host toolchain fits, but only as fallback.
                    level = std::min(item.matchTarget(tcAbi), DebuggerItem::MatchesSomewhat);
                }
            } else {
                // We have an executable path.
                FileName fileName = FileName::fromUserInput(binary);
                if (item.command() == fileName) {
                    // And it's is the path of this item.
                    level = std::min(item.matchTarget(tcAbi), DebuggerItem::MatchesSomewhat);
                } else {
                    // This item does not match by filename, and is an unlikely candidate.
                    // However, consider using it as fallback if the tool chain fits.
                    level = std::min(item.matchTarget(tcAbi), DebuggerItem::MatchesSomewhat);
                }
            }
        }

        if (level > bestLevel) {
            bestLevel = level;
            bestItem = item;
        }
    }

    // Use the best id we found, or an invalid one.
    k->setValue(DebuggerKitInformation::id(), bestLevel != DebuggerItem::DoesNotMatch ? bestItem.id() : QVariant());
}


// This handles the upgrade path from 2.8 to 3.0
void DebuggerKitInformation::fix(Kit *k)
{
    // This can be Id, binary path, but not "auto" anymore.
    const QVariant rawId = k->value(DebuggerKitInformation::id());

    if (rawId.isNull()) // No debugger set, that is fine.
        return;

    if (rawId.type() == QVariant::String) {
        if (!DebuggerItemManager::findById(rawId)) {
            qWarning("Unknown debugger id %s in kit %s",
                     qPrintable(rawId.toString()), qPrintable(k->displayName()));
            k->setValue(DebuggerKitInformation::id(), QVariant());
        }
        return; // All fine (now).
    }

    QMap<QString, QVariant> map = rawId.toMap();
    QString binary = map.value(QLatin1String("Binary")).toString();
    if (binary == QLatin1String("auto")) {
        // This should not happen as "auto" is handled by setup() already.
        QTC_CHECK(false);
        k->setValue(DebuggerKitInformation::id(), QVariant());
        return;
    }

    FileName fileName = FileName::fromUserInput(binary);
    const DebuggerItem *item = DebuggerItemManager::findByCommand(fileName);
    if (!item) {
        qWarning("Debugger command %s invalid in kit %s",
                 qPrintable(binary), qPrintable(k->displayName()));
        k->setValue(DebuggerKitInformation::id(), QVariant());
        return;
    }

    k->setValue(DebuggerKitInformation::id(), item->id());
}

// Check the configuration errors and return a flag mask. Provide a quick check and
// a verbose one with a list of errors.

DebuggerKitInformation::ConfigurationErrors DebuggerKitInformation::configurationErrors(const Kit *k)
{
    QTC_ASSERT(k, return NoDebugger);

    const DebuggerItem *item = DebuggerKitInformation::debugger(k);
    if (!item)
        return NoDebugger;

    if (item->command().isEmpty())
        return NoDebugger;

    ConfigurationErrors result = NoConfigurationError;
    const QFileInfo fi = item->command().toFileInfo();
    if (!fi.exists() || fi.isDir())
        result |= DebuggerNotFound;
    else if (!fi.isExecutable())
        result |= DebuggerNotExecutable;

    const Abi tcAbi = ToolChainKitInformation::targetAbi(k);
    if (item->matchTarget(tcAbi) == DebuggerItem::DoesNotMatch) {
        // currently restricting the check to desktop devices, may be extended to all device types
        const IDevice::ConstPtr device = DeviceKitInformation::device(k);
        if (device && device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
            result |= DebuggerDoesNotMatch;
    }

    if (!fi.exists() || fi.isDir()) {
        if (item->engineType() == NoEngineType)
            return NoDebugger;

        // We need an absolute path to be able to locate Python on Windows.
        if (item->engineType() == GdbEngineType) {
            if (tcAbi.os() == Abi::WindowsOS && !fi.isAbsolute())
                result |= DebuggerNeedsAbsolutePath;
        }
    }
    return result;
}

const DebuggerItem *DebuggerKitInformation::debugger(const Kit *kit)
{
    QTC_ASSERT(kit, return 0);
    const QVariant id = kit->value(DebuggerKitInformation::id());
    return DebuggerItemManager::findById(id);
}

StandardRunnable DebuggerKitInformation::runnable(const Kit *kit)
{
    StandardRunnable runnable;
    if (const DebuggerItem *item = debugger(kit)) {
        runnable.executable = item->command().toString();
        runnable.workingDirectory = item->workingDirectory().toString();
        runnable.environment = Utils::Environment::systemEnvironment();
        runnable.environment.set("LC_NUMERIC", "C");
    }
    return runnable;
}

QList<Task> DebuggerKitInformation::validateDebugger(const Kit *k)
{
    QList<Task> result;

    const ConfigurationErrors errors = configurationErrors(k);
    if (errors == NoConfigurationError)
        return result;

    QString path;
    if (const DebuggerItem *item = debugger(k))
        path = item->command().toUserOutput();

    const Core::Id id = ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM;
    if (errors & NoDebugger)
        result << Task(Task::Warning, tr("No debugger set up."), FileName(), -1, id);

    if (errors & DebuggerNotFound)
        result << Task(Task::Error, tr("Debugger \"%1\" not found.").arg(path),
                       FileName(), -1, id);
    if (errors & DebuggerNotExecutable)
        result << Task(Task::Error, tr("Debugger \"%1\" not executable.").arg(path), FileName(), -1, id);

    if (errors & DebuggerNeedsAbsolutePath) {
        const QString message =
                tr("The debugger location must be given as an "
                   "absolute path (%1).").arg(path);
        result << Task(Task::Error, message, FileName(), -1, id);
    }

    if (errors & DebuggerDoesNotMatch) {
        const QString message = tr("The ABI of the selected debugger does not "
                                   "match the toolchain ABI.");
        result << Task(Task::Warning, message, FileName(), -1, id);
    }
    return result;
}

KitConfigWidget *DebuggerKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::DebuggerKitConfigWidget(k, this);
}

void DebuggerKitInformation::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    expander->registerVariable("Debugger:Name", tr("Name of Debugger"),
                               [this, kit]() -> QString {
                                   const DebuggerItem *item = debugger(kit);
                                   return item ? item->displayName() : tr("Unknown debugger");
                               });

    expander->registerVariable("Debugger:Type", tr("Type of Debugger Backend"),
                               [this, kit]() -> QString {
                                   const DebuggerItem *item = debugger(kit);
                                   return item ? item->engineTypeName() : tr("Unknown debugger type");
                               });

    expander->registerVariable("Debugger:Version", tr("Debugger"),
                               [this, kit]() -> QString {
                                   const DebuggerItem *item = debugger(kit);
                                   return item && !item->version().isEmpty()
                                        ? item->version() : tr("Unknown debugger version");
                               });

    expander->registerVariable("Debugger:Abi", tr("Debugger"),
                               [this, kit]() -> QString {
                                   const DebuggerItem *item = debugger(kit);
                                   return item && !item->abis().isEmpty()
                                           ? item->abiNames().join(QLatin1Char(' '))
                                           : tr("Unknown debugger ABI");
                               });
}

KitInformation::ItemList DebuggerKitInformation::toUserOutput(const Kit *k) const
{
    return ItemList() << qMakePair(tr("Debugger"), displayString(k));
}

DebuggerEngineType DebuggerKitInformation::engineType(const Kit *k)
{
    const DebuggerItem *item = debugger(k);
    QTC_ASSERT(item, return NoEngineType);
    return item->engineType();
}

QString DebuggerKitInformation::displayString(const Kit *k)
{
    const DebuggerItem *item = debugger(k);
    if (!item)
        return tr("No Debugger");
    QString binary = item->command().toUserOutput();
    QString name = tr("%1 Engine").arg(item->engineTypeName());
    return binary.isEmpty() ? tr("%1 <None>").arg(name) : tr("%1 using \"%2\"").arg(name, binary);
}

void DebuggerKitInformation::setDebugger(Kit *k, const QVariant &id)
{
    // Only register reasonably complete debuggers.
    QTC_ASSERT(DebuggerItemManager::findById(id), return);
    k->setValue(DebuggerKitInformation::id(), id);
}

Core::Id DebuggerKitInformation::id()
{
    return "Debugger.Information";
}

} // namespace Debugger
