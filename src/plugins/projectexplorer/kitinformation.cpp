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

#include "kitinformation.h"

#include "devicesupport/desktopdevice.h"
#include "devicesupport/devicemanager.h"
#include "projectexplorerconstants.h"
#include "kit.h"
#include "kitinformationconfigwidget.h"
#include "toolchain.h"
#include "toolchainmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/abi.h>
#include <ssh/sshconnection.h>

#include <utils/algorithm.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>

namespace ProjectExplorer {

const char KITINFORMATION_ID_V1[] = "PE.Profile.ToolChain";
const char KITINFORMATION_ID_V2[] = "PE.Profile.ToolChains";
const char KITINFORMATION_ID_V3[] = "PE.Profile.ToolChainsV3";

// --------------------------------------------------------------------------
// SysRootKitInformation:
// --------------------------------------------------------------------------

SysRootKitInformation::SysRootKitInformation()
{
    setObjectName(QLatin1String("SysRootInformation"));
    setId(SysRootKitInformation::id());
    setPriority(31000);
}

QVariant SysRootKitInformation::defaultValue(const Kit *k) const
{
    Q_UNUSED(k)
    return QString();
}

QList<Task> SysRootKitInformation::validate(const Kit *k) const
{
    QList<Task> result;
    const Utils::FileName dir = SysRootKitInformation::sysRoot(k);
    if (dir.isEmpty())
        return result;

    if (dir.toString().startsWith("target:") || dir.toString().startsWith("remote:"))
        return result;

    const QFileInfo fi = dir.toFileInfo();

    if (!fi.exists()) {
        result << Task(Task::Warning, tr("Sys Root \"%1\" does not exist in the file system.").arg(dir.toUserOutput()),
                       Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
    } else if (!fi.isDir()) {
        result << Task(Task::Warning, tr("Sys Root \"%1\" is not a directory.").arg(dir.toUserOutput()),
                       Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
    } else if (QDir(dir.toString()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty()) {
        result << Task(Task::Warning, tr("Sys Root \"%1\" is empty.").arg(dir.toUserOutput()),
                       Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    return result;
}

KitConfigWidget *SysRootKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::SysRootInformationConfigWidget(k, this);
}

KitInformation::ItemList SysRootKitInformation::toUserOutput(const Kit *k) const
{
    return ItemList() << qMakePair(tr("Sys Root"), sysRoot(k).toUserOutput());
}

void SysRootKitInformation::addToMacroExpander(Kit *kit, Utils::MacroExpander *expander) const
{
    expander->registerFileVariables("SysRoot", tr("Sys Root"), [kit]() -> QString {
        return SysRootKitInformation::sysRoot(kit).toString();
    });
}

Core::Id SysRootKitInformation::id()
{
    return "PE.Profile.SysRoot";
}

bool SysRootKitInformation::hasSysRoot(const Kit *k)
{
    if (k)
        return !k->value(SysRootKitInformation::id()).toString().isEmpty();
    return false;
}

Utils::FileName SysRootKitInformation::sysRoot(const Kit *k)
{
    if (!k)
        return Utils::FileName();
    return Utils::FileName::fromString(k->value(SysRootKitInformation::id()).toString());
}

void SysRootKitInformation::setSysRoot(Kit *k, const Utils::FileName &v)
{
    if (k)
        k->setValue(SysRootKitInformation::id(), v.toString());
}

// --------------------------------------------------------------------------
// ToolChainKitInformation:
// --------------------------------------------------------------------------

ToolChainKitInformation::ToolChainKitInformation()
{
    setObjectName(QLatin1String("ToolChainInformation"));
    setId(ToolChainKitInformation::id());
    setPriority(30000);

    connect(KitManager::instance(), &KitManager::kitsLoaded,
            this, &ToolChainKitInformation::kitsWereLoaded);
}

// language id -> tool chain id
static QMap<Core::Id, QByteArray> defaultToolChainIds()
{
    QMap<Core::Id, QByteArray> toolChains;
    const Abi abi = Abi::hostAbi();
    QList<ToolChain *> tcList = ToolChainManager::toolChains(Utils::equal(&ToolChain::targetAbi, abi));
    foreach (Core::Id l, ToolChainManager::allLanguages()) {
        ToolChain *tc = Utils::findOrDefault(tcList, Utils::equal(&ToolChain::language, l));
        toolChains.insert(l, tc ? tc->id() : QByteArray());
    }
    return toolChains;
}

static QVariant defaultToolChainValue()
{
    const QMap<Core::Id, QByteArray> toolChains = defaultToolChainIds();
    QVariantMap result;
    auto end = toolChains.end();
    for (auto it = toolChains.begin(); it != end; ++it) {
        result.insert(it.key().toString(), it.value());
    }
    return result;
}

QVariant ToolChainKitInformation::defaultValue(const Kit *k) const
{
    Q_UNUSED(k);
    return defaultToolChainValue();
}

QList<Task> ToolChainKitInformation::validate(const Kit *k) const
{
    QList<Task> result;

    const QList<ToolChain*> tcList = toolChains(k);
    if (tcList.isEmpty()) {
        result << Task(Task::Error, ToolChainKitInformation::msgNoToolChainInTarget(),
                       Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
    } else {
        QSet<Abi> targetAbis;
        foreach (ToolChain *tc, tcList) {
            targetAbis.insert(tc->targetAbi());
            result << tc->validateKit(k);
        }
        if (targetAbis.count() != 1) {
            result << Task(Task::Error, tr("Compilers produce code for different ABIs: %1")
                           .arg(Utils::transform(targetAbis, &Abi::toString).toList().join(", ")),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    }
    return result;
}

void ToolChainKitInformation::upgrade(Kit *k)
{
    const Core::Id oldIdV1 = KITINFORMATION_ID_V1;
    const Core::Id oldIdV2 = KITINFORMATION_ID_V2;

    // upgrade <=4.1 to 4.2 (keep old settings around for now)
    {
        const QVariant oldValue = k->value(oldIdV1);
        const QVariant value = k->value(oldIdV2);
        if (value.isNull() && !oldValue.isNull()) {
            QVariantMap newValue;
            if (oldValue.type() == QVariant::Map) {
                // Used between 4.1 and 4.2:
                newValue = oldValue.toMap();
            } else {
                // Used up to 4.1:
                newValue.insert(Deprecated::Toolchain::languageId(Deprecated::Toolchain::Cxx), oldValue.toString());

                const Core::Id typeId = DeviceTypeKitInformation::deviceTypeId(k);
                if (typeId == Constants::DESKTOP_DEVICE_TYPE) {
                    // insert default C compiler which did not exist before
                    newValue.insert(Deprecated::Toolchain::languageId(Deprecated::Toolchain::C),
                                    defaultToolChainIds().value(Core::Id(Constants::C_LANGUAGE_ID)));
                }
            }
            k->setValue(oldIdV2, newValue);
            k->setSticky(oldIdV2, k->isSticky(oldIdV1));
        }
    }

    // upgrade 4.2 to 4.3 (keep old settings around for now)
    {
        const QVariant oldValue = k->value(oldIdV2);
        const QVariant value = k->value(ToolChainKitInformation::id());
        if (value.isNull() && !oldValue.isNull()) {
            QVariantMap newValue = oldValue.toMap();
            QVariantMap::iterator it = newValue.find(Deprecated::Toolchain::languageId(Deprecated::Toolchain::C));
            if (it != newValue.end())
                newValue.insert(Core::Id(Constants::C_LANGUAGE_ID).toString(), it.value());
            it = newValue.find(Deprecated::Toolchain::languageId(Deprecated::Toolchain::Cxx));
            if (it != newValue.end())
                newValue.insert(Core::Id(Constants::CXX_LANGUAGE_ID).toString(), it.value());
            k->setValue(ToolChainKitInformation::id(), newValue);
            k->setSticky(ToolChainKitInformation::id(), k->isSticky(oldIdV2));
        }
    }

    // upgrade 4.3-temporary-master-state to 4.3:
    {
        const QVariantMap valueMap = k->value(ToolChainKitInformation::id()).toMap();
        QVariantMap result;
        for (const QString &key : valueMap.keys()) {
            const int pos = key.lastIndexOf('.');
            if (pos >= 0)
                result.insert(key.mid(pos + 1), valueMap.value(key));
            else
                result.insert(key, valueMap.value(key));
        }
        k->setValue(ToolChainKitInformation::id(), result);
    }
}

void ToolChainKitInformation::fix(Kit *k)
{
    QTC_ASSERT(ToolChainManager::isLoaded(), return);
    foreach (const Core::Id& l, ToolChainManager::allLanguages()) {
        if (!toolChain(k, l)) {
            qWarning("No tool chain set up in kit \"%s\" for \"%s\".",
                     qPrintable(k->displayName()),
                     qPrintable(ToolChainManager::displayNameOfLanguageId(l)));
            clearToolChain(k, l); // make sure to clear out no longer known tool chains
        }
    }
}

static Core::Id findLanguage(const QString &ls)
{
    QString lsUpper = ls.toUpper();
    return Utils::findOrDefault(ToolChainManager::allLanguages(),
                         [lsUpper](Core::Id l) { return lsUpper == l.toString().toUpper(); });
}

void ToolChainKitInformation::setup(Kit *k)
{
    QTC_ASSERT(ToolChainManager::isLoaded(), return);
    const QVariantMap value = k->value(ToolChainKitInformation::id()).toMap();

    for (auto i = value.constBegin(); i != value.constEnd(); ++i) {
        Core::Id l = findLanguage(i.key());

        if (!l.isValid())
            continue;

        const QByteArray id = i.value().toByteArray();
        ToolChain *tc = ToolChainManager::findToolChain(id);
        if (tc)
            continue;

        // ID is not found: Might be an ABI string...
        const QString abi = QString::fromUtf8(id);
        tc = ToolChainManager::toolChain([abi, l](const ToolChain *t) {
                 return t->targetAbi().toString() == abi && t->language() == l;
             });
        if (tc)
            setToolChain(k, tc);
        else
            clearToolChain(k, l);
    }
}

KitConfigWidget *ToolChainKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::ToolChainInformationConfigWidget(k, this);
}

QString ToolChainKitInformation::displayNamePostfix(const Kit *k) const
{
    ToolChain *tc = toolChain(k, Constants::CXX_LANGUAGE_ID);
    return tc ? tc->displayName() : QString();
}

KitInformation::ItemList ToolChainKitInformation::toUserOutput(const Kit *k) const
{
    ToolChain *tc = toolChain(k, Constants::CXX_LANGUAGE_ID);
    return ItemList() << qMakePair(tr("Compiler"), tc ? tc->displayName() : tr("None"));
}

void ToolChainKitInformation::addToEnvironment(const Kit *k, Utils::Environment &env) const
{
    ToolChain *tc = toolChain(k, Constants::CXX_LANGUAGE_ID);
    if (tc)
        tc->addToEnvironment(env);
}

void ToolChainKitInformation::addToMacroExpander(Kit *kit, Utils::MacroExpander *expander) const
{
    // Compatibility with Qt Creator < 4.2:
    expander->registerVariable("Compiler:Name", tr("Compiler"),
                               [kit]() -> QString {
                                   const ToolChain *tc = toolChain(kit, Constants::CXX_LANGUAGE_ID);
                                   return tc ? tc->displayName() : tr("None");
                               });

    expander->registerVariable("Compiler:Executable", tr("Path to the compiler executable"),
                               [kit]() -> QString {
                                   const ToolChain *tc = toolChain(kit, Constants::CXX_LANGUAGE_ID);
                                   return tc ? tc->compilerCommand().toString() : QString();
                               });

    expander->registerPrefix("Compiler:Name", tr("Compiler for different languages"),
                             [kit](const QString &ls) -> QString {
                                 const ToolChain *tc = toolChain(kit, findLanguage(ls));
                                 return tc ? tc->displayName() : tr("None");
                             });
    expander->registerPrefix("Compiler:Executable", tr("Compiler executable for different languages"),
                             [kit](const QString &ls) -> QString {
                                 const ToolChain *tc = toolChain(kit, findLanguage(ls));
                                 return tc ? tc->compilerCommand().toString() : QString();
                             });
}


IOutputParser *ToolChainKitInformation::createOutputParser(const Kit *k) const
{
    ToolChain *tc = toolChain(k, Constants::CXX_LANGUAGE_ID);
    if (tc)
        return tc->outputParser();
    return 0;
}

QSet<Core::Id> ToolChainKitInformation::availableFeatures(const Kit *k) const
{
    QSet<Core::Id> result;
    for (ToolChain *tc : toolChains(k))
        result.insert(tc->typeId().withPrefix("ToolChain."));
    return result;
}

Core::Id ToolChainKitInformation::id()
{
    return KITINFORMATION_ID_V3;
}

ToolChain *ToolChainKitInformation::toolChain(const Kit *k, Core::Id language)
{
    QTC_ASSERT(ToolChainManager::isLoaded(), return 0);
    if (!k)
        return 0;
    QVariantMap value = k->value(ToolChainKitInformation::id()).toMap();
    const QByteArray id = value.value(language.toString(), QByteArray()).toByteArray();
    return ToolChainManager::findToolChain(id);
}

QList<ToolChain *> ToolChainKitInformation::toolChains(const Kit *k)
{
    const QVariantMap value = k->value(ToolChainKitInformation::id()).toMap();
    const QList<ToolChain *> tcList
            = Utils::transform(ToolChainManager::allLanguages().toList(),
                               [&value](Core::Id l) -> ToolChain * {
                                   return ToolChainManager::findToolChain(value.value(l.toString()).toByteArray());
                               });
    return Utils::filtered(tcList, [](ToolChain *tc) { return tc; });
}

void ToolChainKitInformation::setToolChain(Kit *k, ToolChain *tc)
{
    QTC_ASSERT(tc, return);
    QVariantMap result = k->value(ToolChainKitInformation::id()).toMap();
    result.insert(tc->language().toString(), tc->id());

    k->setValue(id(), result);
}

/**
 * @brief ToolChainKitInformation::setAllToolChainsToMatch
 *
 * Set up all toolchains to be similar to the one toolchain provided. Similar ideally means
 * that all toolchains use the "same" compiler from the same installation, but we will
 * settle for a toolchain with a matching API instead.
 *
 * @param k The kit to set up
 * @param tc The toolchain to match other languages for.
 */
void ToolChainKitInformation::setAllToolChainsToMatch(Kit *k, ToolChain *tc)
{
    QTC_ASSERT(tc, return);

    const QList<ToolChain *> allTcList = ToolChainManager::toolChains();
    QTC_ASSERT(allTcList.contains(tc), return);

    QVariantMap result = k->value(ToolChainKitInformation::id()).toMap();
    result.insert(tc->language().toString(), tc->id());

    for (Core::Id l : ToolChainManager::allLanguages()) {
        if (l == tc->language())
            continue;

        ToolChain *match = nullptr;
        ToolChain *bestMatch = nullptr;
        for (ToolChain *other : allTcList) {
            if (!other->isValid() || other->language() != l)
                continue;
            if (other->targetAbi() == tc->targetAbi())
                match = other;
            if (match == other
                    && other->compilerCommand().parentDir() == tc->compilerCommand().parentDir()) {
                bestMatch = other;
                break;
            }
        }
        if (bestMatch)
            result.insert(l.toString(), bestMatch->id());
        else if (match)
            result.insert(l.toString(), match->id());
        else
            result.insert(l.toString(), QByteArray());
    }

    k->setValue(id(), result);
}

void ToolChainKitInformation::clearToolChain(Kit *k, Core::Id language)
{
    QTC_ASSERT(language.isValid(), return);

    QVariantMap result = k->value(ToolChainKitInformation::id()).toMap();
    result.insert(language.toString(), QByteArray());
    k->setValue(id(), result);
}

Abi ToolChainKitInformation::targetAbi(const Kit *k)
{
    QList<ToolChain *> tcList = toolChains(k);
    // Find the best possible ABI for all the tool chains...
    Abi cxxAbi;
    QHash<Abi, int> abiCount;
    foreach (ToolChain *tc, tcList) {
        Abi ta = tc->targetAbi();
        if (tc->language() == Core::Id(Constants::CXX_LANGUAGE_ID))
            cxxAbi = tc->targetAbi();
        abiCount[ta] = (abiCount.contains(ta) ? abiCount[ta] + 1 : 1);
    }
    QVector<Abi> candidates;
    int count = -1;
    candidates.reserve(tcList.count());
    for (auto i = abiCount.begin(); i != abiCount.end(); ++i) {
        if (i.value() > count) {
            candidates.clear();
            candidates.append(i.key());
            count = i.value();
        } else if (i.value() == count) {
            candidates.append(i.key());
        }
    }

    // Found a good candidate:
    if (candidates.isEmpty())
        return Abi::hostAbi();
    if (candidates.contains(cxxAbi)) // Use Cxx compiler as a tie breaker
        return cxxAbi;
    return candidates.at(0); // Use basically a random Abi...
}

QString ToolChainKitInformation::msgNoToolChainInTarget()
{
    return tr("No compiler set in kit.");
}

void ToolChainKitInformation::kitsWereLoaded()
{
    foreach (Kit *k, KitManager::kits())
        fix(k);

    connect(ToolChainManager::instance(), &ToolChainManager::toolChainRemoved,
            this, &ToolChainKitInformation::toolChainRemoved);
    connect(ToolChainManager::instance(), &ToolChainManager::toolChainUpdated,
            this, &ToolChainKitInformation::toolChainUpdated);
}

void ToolChainKitInformation::toolChainUpdated(ToolChain *tc)
{
    for (Kit *k : KitManager::kits([tc](const Kit *k) { return toolChain(k, tc->language()) == tc; }))
        notifyAboutUpdate(k);
}

void ToolChainKitInformation::toolChainRemoved(ToolChain *tc)
{
    Q_UNUSED(tc);
    foreach (Kit *k, KitManager::kits())
        fix(k);
}

// --------------------------------------------------------------------------
// DeviceTypeKitInformation:
// --------------------------------------------------------------------------

DeviceTypeKitInformation::DeviceTypeKitInformation()
{
    setObjectName(QLatin1String("DeviceTypeInformation"));
    setId(DeviceTypeKitInformation::id());
    setPriority(33000);
}

QVariant DeviceTypeKitInformation::defaultValue(const Kit *k) const
{
    Q_UNUSED(k);
    return QByteArray(Constants::DESKTOP_DEVICE_TYPE);
}

QList<Task> DeviceTypeKitInformation::validate(const Kit *k) const
{
    Q_UNUSED(k);
    return QList<Task>();
}

KitConfigWidget *DeviceTypeKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::DeviceTypeInformationConfigWidget(k, this);
}

KitInformation::ItemList DeviceTypeKitInformation::toUserOutput(const Kit *k) const
{
    Core::Id type = deviceTypeId(k);
    QString typeDisplayName = tr("Unknown device type");
    if (type.isValid()) {
        IDeviceFactory *factory = ExtensionSystem::PluginManager::getObject<IDeviceFactory>(
            [&type](IDeviceFactory *factory) {
                return factory->availableCreationIds().contains(type);
            });

        if (factory)
            typeDisplayName = factory->displayNameForId(type);
    }
    return ItemList() << qMakePair(tr("Device type"), typeDisplayName);
}

const Core::Id DeviceTypeKitInformation::id()
{
    return "PE.Profile.DeviceType";
}

const Core::Id DeviceTypeKitInformation::deviceTypeId(const Kit *k)
{
    return k ? Core::Id::fromSetting(k->value(DeviceTypeKitInformation::id())) : Core::Id();
}

void DeviceTypeKitInformation::setDeviceTypeId(Kit *k, Core::Id type)
{
    k->setValue(DeviceTypeKitInformation::id(), type.toSetting());
}

Kit::Predicate DeviceTypeKitInformation::deviceTypePredicate(Core::Id type)
{
    return [type](const Kit *kit) { return type.isValid() && deviceTypeId(kit) == type; };
}

QSet<Core::Id> DeviceTypeKitInformation::supportedPlatforms(const Kit *k) const
{
    return {deviceTypeId(k)};
}

QSet<Core::Id> DeviceTypeKitInformation::availableFeatures(const Kit *k) const
{
    Core::Id id = DeviceTypeKitInformation::deviceTypeId(k);
    if (id.isValid())
        return {id.withPrefix("DeviceType.")};
    return QSet<Core::Id>();
}

// --------------------------------------------------------------------------
// DeviceKitInformation:
// --------------------------------------------------------------------------

DeviceKitInformation::DeviceKitInformation()
{
    setObjectName(QLatin1String("DeviceInformation"));
    setId(DeviceKitInformation::id());
    setPriority(32000);

    connect(KitManager::instance(), &KitManager::kitsLoaded,
            this, &DeviceKitInformation::kitsWereLoaded);
}

QVariant DeviceKitInformation::defaultValue(const Kit *k) const
{
    Core::Id type = DeviceTypeKitInformation::deviceTypeId(k);
    // Use default device if that is compatible:
    IDevice::ConstPtr dev = DeviceManager::instance()->defaultDevice(type);
    if (dev && dev->isCompatibleWith(k))
        return dev->id().toString();
    // Use any other device that is compatible:
    for (int i = 0; i < DeviceManager::instance()->deviceCount(); ++i) {
        dev = DeviceManager::instance()->deviceAt(i);
        if (dev && dev->isCompatibleWith(k))
            return dev->id().toString();
    }
    // Fail: No device set up.
    return QString();
}

QList<Task> DeviceKitInformation::validate(const Kit *k) const
{
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    QList<Task> result;
    if (dev.isNull())
        result.append(Task(Task::Warning, tr("No device set."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)));
    else if (!dev->isCompatibleWith(k))
        result.append(Task(Task::Error, tr("Device is incompatible with this kit."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)));

    return result;
}

void DeviceKitInformation::fix(Kit *k)
{
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    if (!dev.isNull() && !dev->isCompatibleWith(k)) {
        qWarning("Device is no longer compatible with kit \"%s\", removing it.",
                 qPrintable(k->displayName()));
        setDeviceId(k, Core::Id());
    }
}

void DeviceKitInformation::setup(Kit *k)
{
    QTC_ASSERT(DeviceManager::instance()->isLoaded(), return);
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    if (!dev.isNull() && dev->isCompatibleWith(k))
        return;

    setDeviceId(k, Core::Id::fromSetting(defaultValue(k)));
}

KitConfigWidget *DeviceKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::DeviceInformationConfigWidget(k, this);
}

QString DeviceKitInformation::displayNamePostfix(const Kit *k) const
{
    IDevice::ConstPtr dev = device(k);
    return dev.isNull() ? QString() : dev->displayName();
}

KitInformation::ItemList DeviceKitInformation::toUserOutput(const Kit *k) const
{
    IDevice::ConstPtr dev = device(k);
    return ItemList() << qMakePair(tr("Device"), dev.isNull() ? tr("Unconfigured") : dev->displayName());
}

void DeviceKitInformation::addToMacroExpander(Kit *kit, Utils::MacroExpander *expander) const
{
    expander->registerVariable("Device:HostAddress", tr("Host address"),
        [kit]() -> QString {
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? device->sshParameters().host : QString();
    });
    expander->registerVariable("Device:SshPort", tr("SSH port"),
        [kit]() -> QString {
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? QString::number(device->sshParameters().port) : QString();
    });
    expander->registerVariable("Device:UserName", tr("User name"),
        [kit]() -> QString {
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? device->sshParameters().userName : QString();
    });
    expander->registerVariable("Device:KeyFile", tr("Private key file"),
        [kit]() -> QString {
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? device->sshParameters().privateKeyFile : QString();
    });
    expander->registerVariable("Device:Name", tr("Device name"),
        [kit]() -> QString {
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? device->displayName() : QString();
    });
}

Core::Id DeviceKitInformation::id()
{
    return "PE.Profile.Device";
}

IDevice::ConstPtr DeviceKitInformation::device(const Kit *k)
{
    QTC_ASSERT(DeviceManager::instance()->isLoaded(), return IDevice::ConstPtr());
    return DeviceManager::instance()->find(deviceId(k));
}

Core::Id DeviceKitInformation::deviceId(const Kit *k)
{
    return k ? Core::Id::fromSetting(k->value(DeviceKitInformation::id())) : Core::Id();
}

void DeviceKitInformation::setDevice(Kit *k, IDevice::ConstPtr dev)
{
    setDeviceId(k, dev ? dev->id() : Core::Id());
}

void DeviceKitInformation::setDeviceId(Kit *k, Core::Id id)
{
    k->setValue(DeviceKitInformation::id(), id.toSetting());
}

void DeviceKitInformation::kitsWereLoaded()
{
    foreach (Kit *k, KitManager::kits())
        fix(k);

    DeviceManager *dm = DeviceManager::instance();
    connect(dm, &DeviceManager::deviceListReplaced, this, &DeviceKitInformation::devicesChanged);
    connect(dm, &DeviceManager::deviceAdded, this, &DeviceKitInformation::devicesChanged);
    connect(dm, &DeviceManager::deviceRemoved, this, &DeviceKitInformation::devicesChanged);
    connect(dm, &DeviceManager::deviceUpdated, this, &DeviceKitInformation::deviceUpdated);

    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &DeviceKitInformation::kitUpdated);
    connect(KitManager::instance(), &KitManager::unmanagedKitUpdated,
            this, &DeviceKitInformation::kitUpdated);
}

void DeviceKitInformation::deviceUpdated(Core::Id id)
{
    foreach (Kit *k, KitManager::kits()) {
        if (deviceId(k) == id)
            notifyAboutUpdate(k);
    }
}

void DeviceKitInformation::kitUpdated(Kit *k)
{
    setup(k); // Set default device if necessary
}

void DeviceKitInformation::devicesChanged()
{
    foreach (Kit *k, KitManager::kits())
        setup(k); // Set default device if necessary
}

// --------------------------------------------------------------------------
// EnvironmentKitInformation:
// --------------------------------------------------------------------------

EnvironmentKitInformation::EnvironmentKitInformation()
{
    setObjectName(QLatin1String("EnvironmentKitInformation"));
    setId(EnvironmentKitInformation::id());
    setPriority(29000);
}

QVariant EnvironmentKitInformation::defaultValue(const Kit *k) const
{
    Q_UNUSED(k)
    return QStringList();
}

QList<Task> EnvironmentKitInformation::validate(const Kit *k) const
{
    QList<Task> result;
    const QVariant variant = k->value(EnvironmentKitInformation::id());
    if (!variant.isNull() && !variant.canConvert(QVariant::List)) {
        result.append(Task(Task::Error, tr("The environment setting value is invalid."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)));
    }
    return result;
}

void EnvironmentKitInformation::fix(Kit *k)
{
    const QVariant variant = k->value(EnvironmentKitInformation::id());
    if (!variant.isNull() && !variant.canConvert(QVariant::List)) {
        qWarning("Kit \"%s\" has a wrong environment value set.", qPrintable(k->displayName()));
        setEnvironmentChanges(k, QList<Utils::EnvironmentItem>());
    }
}

void EnvironmentKitInformation::addToEnvironment(const Kit *k, Utils::Environment &env) const
{
    const QVariant envValue = k->value(EnvironmentKitInformation::id());
    if (envValue.isValid())
        env.modify(Utils::EnvironmentItem::fromStringList(envValue.toStringList()));
}

KitConfigWidget *EnvironmentKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::KitEnvironmentConfigWidget(k, this);
}

KitInformation::ItemList EnvironmentKitInformation::toUserOutput(const Kit *k) const
{
    ItemList retVal;
    QVariant envValue = k->value(EnvironmentKitInformation::id());
    if (envValue.isValid())
        retVal << qMakePair(QLatin1Literal("Environment"), envValue.toStringList().join(QLatin1Literal("<br>")));

    return retVal;
}

Core::Id EnvironmentKitInformation::id()
{
    return "PE.Profile.Environment";
}

QList<Utils::EnvironmentItem> EnvironmentKitInformation::environmentChanges(const Kit *k)
{
     if (k)
         return Utils::EnvironmentItem::fromStringList(k->value(EnvironmentKitInformation::id()).toStringList());
     return QList<Utils::EnvironmentItem>();
}

void EnvironmentKitInformation::setEnvironmentChanges(Kit *k, const QList<Utils::EnvironmentItem> &changes)
{
    if (k)
        k->setValue(EnvironmentKitInformation::id(), Utils::EnvironmentItem::toStringList(changes));
}

} // namespace ProjectExplorer
