/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggerkitinformation.h"

#include "debuggeritemmanager.h"
#include "debuggerkitconfigwidget.h"

#include "projectexplorer/toolchain.h"
#include "projectexplorer/projectexplorerconstants.h"

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

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

QVariant DebuggerKitInformation::defaultValue(Kit *k) const
{
    ToolChain *tc = ToolChainKitInformation::toolChain(k);
    if (!tc)
        return QVariant();

    const Abi toolChainAbi = tc->targetAbi();
    foreach (const DebuggerItem &item, DebuggerItemManager::debuggers())
        foreach (const Abi targetAbi, item.abis())
            if (targetAbi.isCompatibleWith(toolChainAbi))
                return item.id();

    return QVariant();
}

void DebuggerKitInformation::setup(Kit *k)
{
    // Get one of the available debugger matching the kit's toolchain.
    const ToolChain *tc = ToolChainKitInformation::toolChain(k);
    const Abi toolChainAbi = tc ? tc->targetAbi() : Abi::hostAbi();

    // This can be anything (Id, binary path, "auto")
    const QVariant rawId = k->value(DebuggerKitInformation::id());

    enum {
        NotDetected, DetectedAutomatically, DetectedByFile, DetectedById
    } detection = NotDetected;
    DebuggerEngineType autoEngine = NoEngineType;
    FileName fileName;

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

    if (rawId.isNull()) {
        // Initial setup of a kit
        detection = NotDetected;
    } else if (rawId.type() == QVariant::String) {
        detection = DetectedById;
    } else {
        QMap<QString, QVariant> map = rawId.toMap();
        QString binary = map.value(QLatin1String("Binary")).toString();
        if (binary == QLatin1String("auto")) {
            detection = DetectedAutomatically;
            autoEngine = DebuggerEngineType(map.value(QLatin1String("EngineType")).toInt());
        } else {
            detection  = DetectedByFile;
            fileName = FileName::fromUserInput(binary);
        }
    }

    const DebuggerItem *bestItem = 0;
    DebuggerItem::MatchLevel bestLevel = DebuggerItem::DoesNotMatch;
    foreach (const DebuggerItem &item, DebuggerItemManager::debuggers()) {
        const DebuggerItem *goodItem = 0;
        if (detection == DetectedById && item.id() == rawId)
            goodItem = &item;
        if (detection == DetectedByFile && item.command() == fileName)
            goodItem = &item;
        if (detection == DetectedAutomatically && item.engineType() == autoEngine)
            goodItem = &item;
        if (item.isAutoDetected())
            goodItem = &item;

        if (goodItem) {
            DebuggerItem::MatchLevel level = goodItem->matchTarget(toolChainAbi);
            if (level > bestLevel) {
                bestLevel = level;
                bestItem = goodItem;
            }
        }
    }

    // If we have an existing debugger with matching id _and_
    // matching target ABI we are fine.
    if (bestItem) {
        k->setValue(DebuggerKitInformation::id(), bestItem->id());
        return;
    }

    // We didn't find an existing debugger that matched by whatever
    // data we found in the kit (i.e. no id, filename, "auto")
    // (or what we found did not match ABI-wise)
    // Let's try to pick one with matching ABI.
    QVariant bestId;
    bestLevel = DebuggerItem::DoesNotMatch;
    foreach (const DebuggerItem &item, DebuggerItemManager::debuggers()) {
        DebuggerItem::MatchLevel level = item.matchTarget(toolChainAbi);
        if (level > bestLevel) {
            bestLevel = level;
            bestId = item.id();
        }
    }

    k->setValue(DebuggerKitInformation::id(), bestId);
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

enum DebuggerConfigurationErrors {
    NoDebugger = 0x1,
    DebuggerNotFound = 0x2,
    DebuggerNotExecutable = 0x4,
    DebuggerNeedsAbsolutePath = 0x8
};

static unsigned debuggerConfigurationErrors(const Kit *k)
{
    QTC_ASSERT(k, return NoDebugger);

    const DebuggerItem *item = DebuggerKitInformation::debugger(k);
    if (!item)
        return NoDebugger;

    if (item->command().isEmpty())
        return NoDebugger;

    unsigned result = 0;
    const QFileInfo fi = item->command().toFileInfo();
    if (!fi.exists() || fi.isDir())
        result |= DebuggerNotFound;
    else if (!fi.isExecutable())
        result |= DebuggerNotExecutable;

    if (!fi.exists() || fi.isDir()) {
        if (item->engineType() == NoEngineType)
            return NoDebugger;

        // We need an absolute path to be able to locate Python on Windows.
        if (item->engineType() == GdbEngineType)
            if (const ToolChain *tc = ToolChainKitInformation::toolChain(k))
                if (tc->targetAbi().os() == Abi::WindowsOS && !fi.isAbsolute())
                    result |= DebuggerNeedsAbsolutePath;
    }
    return result;
}

const DebuggerItem *DebuggerKitInformation::debugger(const Kit *kit)
{
    QTC_ASSERT(kit, return 0);
    const QVariant id = kit->value(DebuggerKitInformation::id());
    return DebuggerItemManager::findById(id);
}

bool DebuggerKitInformation::isValidDebugger(const Kit *k)
{
    return debuggerConfigurationErrors(k) == 0;
}

QList<Task> DebuggerKitInformation::validateDebugger(const Kit *k)
{
    QList<Task> result;

    const unsigned errors = debuggerConfigurationErrors(k);
    if (!errors)
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
    return result;
}

KitConfigWidget *DebuggerKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::DebuggerKitConfigWidget(k, this);
}

KitInformation::ItemList DebuggerKitInformation::toUserOutput(const Kit *k) const
{
    return ItemList() << qMakePair(tr("Debugger"), displayString(k));
}

FileName DebuggerKitInformation::debuggerCommand(const ProjectExplorer::Kit *k)
{
    const DebuggerItem *item = debugger(k);
    QTC_ASSERT(item, return FileName());
    return item->command();
}

DebuggerEngineType DebuggerKitInformation::engineType(const ProjectExplorer::Kit *k)
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
