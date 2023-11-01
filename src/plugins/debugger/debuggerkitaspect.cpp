// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggerkitaspect.h"

#include "debuggeritemmanager.h"
#include "debuggeritem.h"
#include "debuggertr.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>

#include <utils/environment.h>
#include <utils/guard.h>
#include <utils/filepath.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>

#include <QComboBox>

#include <utility>

using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger {

// --------------------------------------------------------------------------
// DebuggerKitAspect
// --------------------------------------------------------------------------

namespace Internal {

class DebuggerKitAspectImpl final : public KitAspect
{
public:
    DebuggerKitAspectImpl(Kit *workingCopy, const KitAspectFactory *factory)
        : KitAspect(workingCopy, factory)
    {
        m_comboBox = createSubWidget<QComboBox>();
        m_comboBox->setSizePolicy(QSizePolicy::Ignored, m_comboBox->sizePolicy().verticalPolicy());
        m_comboBox->setEnabled(true);

        refresh();
        m_comboBox->setToolTip(factory->description());
        connect(m_comboBox, &QComboBox::currentIndexChanged, this, [this] {
            if (m_ignoreChanges.isLocked())
                return;

            int currentIndex = m_comboBox->currentIndex();
            QVariant id = m_comboBox->itemData(currentIndex);
            m_kit->setValue(DebuggerKitAspect::id(), id);
        });

        m_manageButton = createManageButton(ProjectExplorer::Constants::DEBUGGER_SETTINGS_PAGE_ID);
    }

    ~DebuggerKitAspectImpl() override
    {
        delete m_comboBox;
        delete m_manageButton;
    }

private:
    void addToLayoutImpl(Layouting::LayoutItem &parent) override
    {
        addMutableAction(m_comboBox);
        parent.addItem(m_comboBox);
        parent.addItem(m_manageButton);
    }

    void makeReadOnly() override
    {
        m_manageButton->setEnabled(false);
        m_comboBox->setEnabled(false);
    }

    void refresh() override
    {
        const GuardLocker locker(m_ignoreChanges);
        m_comboBox->clear();
        m_comboBox->addItem(Tr::tr("None"), QString());

        IDeviceConstPtr device = BuildDeviceKitAspect::device(kit());
        const Utils::FilePath path = device->rootPath();
        const QList<DebuggerItem> list = DebuggerItemManager::debuggers();

        const QList<DebuggerItem> same = Utils::filtered(list, [path](const DebuggerItem &item) {
            return item.command().isSameDevice(path);
        });
        const QList<DebuggerItem> other = Utils::filtered(list, [path](const DebuggerItem &item) {
            return !item.command().isSameDevice(path);
        });

        for (const DebuggerItem &item : same)
            m_comboBox->addItem(item.displayName(), item.id());

        if (!same.isEmpty() && !other.isEmpty())
            m_comboBox->insertSeparator(m_comboBox->count());

        for (const DebuggerItem &item : other)
            m_comboBox->addItem(item.displayName(), item.id());

        const DebuggerItem *item = DebuggerKitAspect::debugger(m_kit);
        updateComboBox(item ? item->id() : QVariant());
    }

    QVariant currentId() const { return m_comboBox->itemData(m_comboBox->currentIndex()); }

    void updateComboBox(const QVariant &id)
    {
        for (int i = 0; i < m_comboBox->count(); ++i) {
            if (id == m_comboBox->itemData(i)) {
                m_comboBox->setCurrentIndex(i);
                return;
            }
        }
        m_comboBox->setCurrentIndex(0);
    }

    Guard m_ignoreChanges;
    QComboBox *m_comboBox;
    QWidget *m_manageButton;
};
} // namespace Internal

// Check the configuration errors and return a flag mask. Provide a quick check and
// a verbose one with a list of errors.

DebuggerKitAspect::ConfigurationErrors DebuggerKitAspect::configurationErrors(const Kit *k)
{
    QTC_ASSERT(k, return NoDebugger);

    const DebuggerItem *item = DebuggerKitAspect::debugger(k);
    if (!item)
        return NoDebugger;

    const FilePath debugger = item->command();
    if (debugger.isEmpty())
        return NoDebugger;

    if (debugger.isRelativePath())
        return NoConfigurationError;

    ConfigurationErrors result = NoConfigurationError;
    if (!debugger.isExecutableFile())
        result |= DebuggerNotExecutable;

    const Abi tcAbi = ToolChainKitAspect::targetAbi(k);
    if (item->matchTarget(tcAbi) == DebuggerItem::DoesNotMatch) {
        // currently restricting the check to desktop devices, may be extended to all device types
        const IDevice::ConstPtr device = DeviceKitAspect::device(k);
        if (device && device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
            result |= DebuggerDoesNotMatch;
    }

    if (item->engineType() == NoEngineType)
        return NoDebugger;

    // We need an absolute path to be able to locate Python on Windows.
    if (item->engineType() == GdbEngineType) {
        if (tcAbi.os() == Abi::WindowsOS && !debugger.isAbsolutePath())
            result |= DebuggerNeedsAbsolutePath;
    }

    return result;
}

const DebuggerItem *DebuggerKitAspect::debugger(const Kit *kit)
{
    QTC_ASSERT(kit, return nullptr);
    const QVariant id = kit->value(DebuggerKitAspect::id());
    return DebuggerItemManager::findById(id);
}

ProcessRunData DebuggerKitAspect::runnable(const Kit *kit)
{
    ProcessRunData runnable;
    if (const DebuggerItem *item = debugger(kit)) {
        FilePath cmd = item->command();
        if (cmd.isRelativePath()) {
            if (const IDeviceConstPtr buildDevice = BuildDeviceKitAspect::device(kit))
                cmd = buildDevice->searchExecutableInPath(cmd.path());
        }
        runnable.command.setExecutable(cmd);
        runnable.workingDirectory = item->workingDirectory();
        runnable.environment = cmd.deviceEnvironment();
        runnable.environment.set("LC_NUMERIC", "C");
    }
    return runnable;
}

Tasks DebuggerKitAspect::validateDebugger(const Kit *k)
{
    Tasks result;

    const ConfigurationErrors errors = configurationErrors(k);
    if (errors == NoConfigurationError)
        return result;

    QString path;
    if (const DebuggerItem *item = debugger(k))
        path = item->command().toUserOutput();

    if (errors & NoDebugger)
        result << BuildSystemTask(Task::Warning, Tr::tr("No debugger set up."));

    if (errors & DebuggerNotFound)
        result << BuildSystemTask(Task::Error, Tr::tr("Debugger \"%1\" not found.").arg(path));

    if (errors & DebuggerNotExecutable)
        result << BuildSystemTask(Task::Error, Tr::tr("Debugger \"%1\" not executable.").arg(path));

    if (errors & DebuggerNeedsAbsolutePath) {
        const QString message =
                Tr::tr("The debugger location must be given as an "
                   "absolute path (%1).").arg(path);
        result << BuildSystemTask(Task::Error, message);
    }

    if (errors & DebuggerDoesNotMatch) {
        const QString message = Tr::tr("The ABI of the selected debugger does not "
                                   "match the toolchain ABI.");
        result << BuildSystemTask(Task::Warning, message);
    }
    return result;
}


DebuggerEngineType DebuggerKitAspect::engineType(const Kit *k)
{
    const DebuggerItem *item = debugger(k);
    QTC_ASSERT(item, return NoEngineType);
    return item->engineType();
}

QString DebuggerKitAspect::displayString(const Kit *k)
{
    const DebuggerItem *item = debugger(k);
    if (!item)
        return Tr::tr("No Debugger");
    QString binary = item->command().toUserOutput();
    QString name = Tr::tr("%1 Engine").arg(item->engineTypeName());
    return binary.isEmpty() ? Tr::tr("%1 <None>").arg(name) : Tr::tr("%1 using \"%2\"").arg(name, binary);
}

QString DebuggerKitAspect::version(const Kit *k)
{
    const DebuggerItem *item = debugger(k);
    QTC_ASSERT(item, return {});
    return item->version();
}

void DebuggerKitAspect::setDebugger(Kit *k, const QVariant &id)
{
    // Only register reasonably complete debuggers.
    QTC_ASSERT(DebuggerItemManager::findById(id), return);
    QTC_ASSERT(k, return);
    k->setValue(DebuggerKitAspect::id(), id);
}

Utils::Id DebuggerKitAspect::id()
{
    return "Debugger.Information";
}

// DebuggerKitAspectFactory

class DebuggerKitAspectFactory : public KitAspectFactory
{
public:
    DebuggerKitAspectFactory()
    {
        setId(DebuggerKitAspect::id());
        setDisplayName(Tr::tr("Debugger"));
        setDescription(Tr::tr("The debugger to use for this kit."));
        setPriority(28000);
    }

    Tasks validate(const Kit *k) const override
    {
        return DebuggerKitAspect::validateDebugger(k);
    }

    void setup(Kit *k) override
    {
        QTC_ASSERT(k, return);

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
        const QVariant rawId = k->value(DebuggerKitAspectFactory::id());

        const Abi tcAbi = ToolChainKitAspect::targetAbi(k);

        // Get the best of the available debugger matching the kit's toolchain.
        // The general idea is to find an item that exactly matches what
        // is stored in the kit information, but also accept item based
        // on toolchain matching as fallback with a lower priority.

        DebuggerItem bestItem;
        DebuggerItem::MatchLevel bestLevel = DebuggerItem::DoesNotMatch;
        const Environment systemEnvironment = Environment::systemEnvironment();
        for (const DebuggerItem &item : DebuggerItemManager::debuggers()) {
            DebuggerItem::MatchLevel level = DebuggerItem::DoesNotMatch;

            if (rawId.isNull()) {
                // Initial setup of a kit.
                level = item.matchTarget(tcAbi);
                // Hack to prefer a debugger from PATH (e.g. autodetected) over other matches.
                // This improves the situation a bit if a cross-compilation tool chain has the
                // same ABI as the host.
                if (level == DebuggerItem::MatchesPerfectly
                    && !item.command().needsDevice()
                    && systemEnvironment.path().contains(item.command().parentDir())) {
                    level = DebuggerItem::MatchesPerfectlyInPath;
                }
                if (!item.detectionSource().isEmpty() && item.detectionSource() == k->autoDetectionSource())
                    level = DebuggerItem::MatchLevel(level + 2);
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
                QString binary = map.value("Binary").toString();
                if (binary == "auto") {
                    // This is close to the "new kit" case, except that we know
                    // an engine type.
                    DebuggerEngineType autoEngine = DebuggerEngineType(map.value("EngineType").toInt());
                    if (item.engineType() == autoEngine) {
                        // Use item if host toolchain fits, but only as fallback.
                        level = std::min(item.matchTarget(tcAbi), DebuggerItem::MatchesSomewhat);
                    }
                } else {
                    // We have an executable path.
                    FilePath fileName = FilePath::fromUserInput(binary);
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
            } else if (level == bestLevel) {
                if (item.engineType() == bestItem.engineType()) {
                    const QStringList itemVersion = item.version().split('.');
                    const QStringList bestItemVersion = bestItem.version().split('.');
                    int end = qMax(item.version().size(), bestItemVersion.size());
                    for (int i = 0; i < end; ++i) {
                        if (itemVersion.value(i) == bestItemVersion.value(i))
                            continue;
                        if (itemVersion.value(i).toInt() > bestItemVersion.value(i).toInt())
                            bestItem = item;
                        break;
                    }
                }
            }
        }

        // Use the best id we found, or an invalid one.
        k->setValue(DebuggerKitAspect::id(), bestLevel != DebuggerItem::DoesNotMatch ? bestItem.id() : QVariant());
    }

    KitAspect *createKitAspect(Kit *k) const override
    {
        return new Internal::DebuggerKitAspectImpl(k, this);
    }

    void addToMacroExpander(Kit *kit, Utils::MacroExpander *expander) const override
    {
        QTC_ASSERT(kit, return);
        expander->registerVariable("Debugger:Name", Tr::tr("Name of Debugger"),
                                   [kit]() -> QString {
                                       const DebuggerItem *item = DebuggerKitAspect::debugger(kit);
                                       return item ? item->displayName() : Tr::tr("Unknown debugger");
                                   });

        expander->registerVariable("Debugger:Type", Tr::tr("Type of Debugger Backend"),
                                   [kit]() -> QString {
                                       const DebuggerItem *item = DebuggerKitAspect::debugger(kit);
                                       return item ? item->engineTypeName() : Tr::tr("Unknown debugger type");
                                   });

        expander->registerVariable("Debugger:Version", Tr::tr("Debugger"),
                                   [kit]() -> QString {
                                       const DebuggerItem *item = DebuggerKitAspect::debugger(kit);
                                       return item && !item->version().isEmpty()
                                                  ? item->version() : Tr::tr("Unknown debugger version");
                                   });

        expander->registerVariable("Debugger:Abi", Tr::tr("Debugger"),
                                   [kit]() -> QString {
                                       const DebuggerItem *item = DebuggerKitAspect::debugger(kit);
                                       return item && !item->abis().isEmpty()
                                                  ? item->abiNames().join(' ')
                                                  : Tr::tr("Unknown debugger ABI");
                                   });
    }

    ItemList toUserOutput(const Kit *k) const override
    {
        return {{Tr::tr("Debugger"), DebuggerKitAspect::displayString(k)}};
    }
};

const DebuggerKitAspectFactory debuggerKitAspectFactory;

} // namespace Debugger
