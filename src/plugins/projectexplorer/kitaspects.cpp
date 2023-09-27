// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "kitaspects.h"

#include "abi.h"
#include "devicesupport/devicemanager.h"
#include "devicesupport/devicemanagermodel.h"
#include "devicesupport/idevicefactory.h"
#include "devicesupport/sshparameters.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "kit.h"
#include "toolchain.h"
#include "toolchainmanager.h"

#include <utils/algorithm.h>
#include <utils/elidinglabel.h>
#include <utils/environment.h>
#include <utils/environmentdialog.h>
#include <utils/guard.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/variablechooser.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFontMetrics>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// SysRootKitAspect:
// --------------------------------------------------------------------------

namespace Internal {
class SysRootKitAspectImpl : public KitAspect
{
public:
    SysRootKitAspectImpl(Kit *k, const KitAspectFactory *factory) : KitAspect(k, factory)
    {
        m_chooser = createSubWidget<PathChooser>();
        m_chooser->setExpectedKind(PathChooser::ExistingDirectory);
        m_chooser->setHistoryCompleter("PE.SysRoot.History");
        m_chooser->setFilePath(SysRootKitAspect::sysRoot(k));
        connect(m_chooser, &PathChooser::textChanged,
                this, &SysRootKitAspectImpl::pathWasChanged);
    }

    ~SysRootKitAspectImpl() override { delete m_chooser; }

private:
    void makeReadOnly() override { m_chooser->setReadOnly(true); }

    void addToLayoutImpl(Layouting::LayoutItem &builder) override
    {
        addMutableAction(m_chooser);
        builder.addItem(Layouting::Span(2, m_chooser));
    }

    void refresh() override
    {
        if (!m_ignoreChanges.isLocked())
            m_chooser->setFilePath(SysRootKitAspect::sysRoot(m_kit));
    }

    void pathWasChanged()
    {
        const GuardLocker locker(m_ignoreChanges);
        SysRootKitAspect::setSysRoot(m_kit, m_chooser->filePath());
    }

    PathChooser *m_chooser;
    Guard m_ignoreChanges;
};
} // namespace Internal

class SysRootKitAspectFactory : public KitAspectFactory
{
public:
    SysRootKitAspectFactory();

    Tasks validate(const Kit *k) const override;
    KitAspect *createKitAspect(Kit *k) const override;
    ItemList toUserOutput(const Kit *k) const override;
    void addToMacroExpander(Kit *kit, MacroExpander *expander) const override;
};

SysRootKitAspectFactory::SysRootKitAspectFactory()
{
    setId(SysRootKitAspect::id());
    setDisplayName(Tr::tr("Sysroot"));
    setDescription(Tr::tr("The root directory of the system image to use.<br>"
                      "Leave empty when building for the desktop."));
    setPriority(27000);
}

Tasks SysRootKitAspectFactory::validate(const Kit *k) const
{
    Tasks result;
    const FilePath dir = SysRootKitAspect::sysRoot(k);
    if (dir.isEmpty())
        return result;

    if (dir.startsWith("target:") || dir.startsWith("remote:"))
        return result;

    if (!dir.exists()) {
        result << BuildSystemTask(Task::Warning,
                    Tr::tr("Sys Root \"%1\" does not exist in the file system.").arg(dir.toUserOutput()));
    } else if (!dir.isDir()) {
        result << BuildSystemTask(Task::Warning,
                    Tr::tr("Sys Root \"%1\" is not a directory.").arg(dir.toUserOutput()));
    } else if (dir.dirEntries(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty()) {
        result << BuildSystemTask(Task::Warning,
                    Tr::tr("Sys Root \"%1\" is empty.").arg(dir.toUserOutput()));
    }
    return result;
}

KitAspect *SysRootKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);

    return new Internal::SysRootKitAspectImpl(k, this);
}

KitAspectFactory::ItemList SysRootKitAspectFactory::toUserOutput(const Kit *k) const
{
    return {{Tr::tr("Sys Root"), SysRootKitAspect::sysRoot(k).toUserOutput()}};
}

void SysRootKitAspectFactory::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    QTC_ASSERT(kit, return);

    expander->registerFileVariables("SysRoot", Tr::tr("Sys Root"), [kit] {
        return SysRootKitAspect::sysRoot(kit);
    });
}

Id SysRootKitAspect::id()
{
    return "PE.Profile.SysRoot";
}

FilePath SysRootKitAspect::sysRoot(const Kit *k)
{
    if (!k)
        return {};

    if (!k->value(SysRootKitAspect::id()).toString().isEmpty())
        return FilePath::fromSettings(k->value(SysRootKitAspect::id()));

    for (ToolChain *tc : ToolChainKitAspect::toolChains(k)) {
        if (!tc->sysRoot().isEmpty())
            return FilePath::fromString(tc->sysRoot());
    }
    return {};
}

void SysRootKitAspect::setSysRoot(Kit *k, const FilePath &v)
{
    if (!k)
        return;

    for (ToolChain *tc : ToolChainKitAspect::toolChains(k)) {
        if (!tc->sysRoot().isEmpty()) {
            // It's the sysroot from toolchain, don't set it.
            if (tc->sysRoot() == v.toString())
                return;

            // We've changed the default toolchain sysroot, set it.
            break;
        }
    }
    k->setValue(SysRootKitAspect::id(), v.toString());
}

const SysRootKitAspectFactory theSyRootKitAspectFactory;

// --------------------------------------------------------------------------
// ToolChainKitAspect:
// --------------------------------------------------------------------------

namespace Internal {
class ToolChainKitAspectImpl final : public KitAspect
{
public:
    ToolChainKitAspectImpl(Kit *k, const KitAspectFactory *factory) : KitAspect(k, factory)
    {
        m_mainWidget = createSubWidget<QWidget>();
        m_mainWidget->setContentsMargins(0, 0, 0, 0);

        auto layout = new QGridLayout(m_mainWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setColumnStretch(1, 2);

        const QList<Id> languageList = sorted(ToolChainManager::allLanguages(), [](Id l1, Id l2) {
            return ToolChainManager::displayNameOfLanguageId(l1)
                    < ToolChainManager::displayNameOfLanguageId(l2);
        });
        QTC_ASSERT(!languageList.isEmpty(), return);
        int row = 0;
        for (Id l : std::as_const(languageList)) {
            layout->addWidget(new QLabel(ToolChainManager::displayNameOfLanguageId(l) + ':'), row, 0);
            auto cb = new QComboBox;
            cb->setSizePolicy(QSizePolicy::Ignored, cb->sizePolicy().verticalPolicy());
            cb->setToolTip(factory->description());

            m_languageComboboxMap.insert(l, cb);
            layout->addWidget(cb, row, 1);
            ++row;

            connect(cb, &QComboBox::currentIndexChanged, this, [this, l](int idx) {
                currentToolChainChanged(l, idx);
            });
        }

        refresh();

        m_manageButton = createManageButton(Constants::TOOLCHAIN_SETTINGS_PAGE_ID);
    }

    ~ToolChainKitAspectImpl() override
    {
        delete m_mainWidget;
        delete m_manageButton;
    }

private:
    void addToLayoutImpl(Layouting::LayoutItem &builder) override
    {
        addMutableAction(m_mainWidget);
        builder.addItem(m_mainWidget);
        builder.addItem(m_manageButton);
    }

    void refresh() override
    {
        IDeviceConstPtr device = BuildDeviceKitAspect::device(kit());

        const GuardLocker locker(m_ignoreChanges);
        const QList<Id> keys = m_languageComboboxMap.keys();
        for (const Id l : keys) {
            const Toolchains ltcList = ToolChainManager::toolchains(equal(&ToolChain::language, l));

            QComboBox *cb = m_languageComboboxMap.value(l);
            cb->clear();
            cb->addItem(Tr::tr("<No compiler>"), QByteArray());

            const QList<ToolChain *> same = Utils::filtered(ltcList, [device](ToolChain *tc) {
                return tc->compilerCommand().isSameDevice(device->rootPath());
            });
            const QList<ToolChain *> other = Utils::filtered(ltcList, [device](ToolChain *tc) {
                return !tc->compilerCommand().isSameDevice(device->rootPath());
            });

            for (ToolChain *item : same)
                cb->addItem(item->displayName(), item->id());

            if (!same.isEmpty() && !other.isEmpty())
                cb->insertSeparator(cb->count());

            for (ToolChain *item : other)
                cb->addItem(item->displayName(), item->id());

            cb->setEnabled(cb->count() > 1 && !m_isReadOnly);
            const int index = indexOf(cb, ToolChainKitAspect::toolChain(m_kit, l));
            cb->setCurrentIndex(index);
        }
    }

    void makeReadOnly() override
    {
        m_isReadOnly = true;
        const QList<Id> keys = m_languageComboboxMap.keys();
        for (const Id l : keys) {
            m_languageComboboxMap.value(l)->setEnabled(false);
        }
    }

    void currentToolChainChanged(Id language, int idx)
    {
        if (m_ignoreChanges.isLocked() || idx < 0)
            return;

        const QByteArray id = m_languageComboboxMap.value(language)->itemData(idx).toByteArray();
        ToolChain *tc = ToolChainManager::findToolChain(id);
        QTC_ASSERT(!tc || tc->language() == language, return);
        if (tc)
            ToolChainKitAspect::setToolChain(m_kit, tc);
        else
            ToolChainKitAspect::clearToolChain(m_kit, language);
    }

    int indexOf(QComboBox *cb, const ToolChain *tc)
    {
        const QByteArray id = tc ? tc->id() : QByteArray();
        for (int i = 0; i < cb->count(); ++i) {
            if (id == cb->itemData(i).toByteArray())
                return i;
        }
        return -1;
    }

    QWidget *m_mainWidget = nullptr;
    QWidget *m_manageButton = nullptr;
    QHash<Id, QComboBox *> m_languageComboboxMap;
    Guard m_ignoreChanges;
    bool m_isReadOnly = false;
};
} // namespace Internal

class ToolChainKitAspectFactory : public KitAspectFactory
{
public:
    ToolChainKitAspectFactory();

private:
    Tasks validate(const Kit *k) const override;
    void fix(Kit *k) override;
    void setup(Kit *k) override;

    KitAspect *createKitAspect(Kit *k) const override;

    QString displayNamePostfix(const Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    void addToBuildEnvironment(const Kit *k, Environment &env) const override;
    void addToRunEnvironment(const Kit *, Environment &) const override {}

    void addToMacroExpander(Kit *kit, MacroExpander *expander) const override;
    QList<OutputLineParser *> createOutputParsers(const Kit *k) const override;
    QSet<Id> availableFeatures(const Kit *k) const override;

    void onKitsLoaded() override;

    void toolChainUpdated(ToolChain *tc);
    void toolChainRemoved(ToolChain *tc);
};

ToolChainKitAspectFactory::ToolChainKitAspectFactory()
{
    setId(ToolChainKitAspect::id());
    setDisplayName(Tr::tr("Compiler"));
    setDescription(Tr::tr("The compiler to use for building.<br>"
                      "Make sure the compiler will produce binaries compatible "
                      "with the target device, Qt version and other libraries used."));
    setPriority(30000);
}

// language id -> tool chain id
static QMap<Id, QByteArray> defaultToolChainIds()
{
    QMap<Id, QByteArray> toolChains;
    const Abi abi = Abi::hostAbi();
    const Toolchains tcList = ToolChainManager::toolchains(equal(&ToolChain::targetAbi, abi));
    const QList<Id> languages = ToolChainManager::allLanguages();
    for (Id l : languages) {
        ToolChain *tc = findOrDefault(tcList, equal(&ToolChain::language, l));
        toolChains.insert(l, tc ? tc->id() : QByteArray());
    }
    return toolChains;
}

static Store defaultToolChainValue()
{
    const QMap<Id, QByteArray> toolChains = defaultToolChainIds();
    Store result;
    auto end = toolChains.end();
    for (auto it = toolChains.begin(); it != end; ++it)
        result.insert(it.key().toKey(), it.value());
    return result;
}

Tasks ToolChainKitAspectFactory::validate(const Kit *k) const
{
    Tasks result;

    const QList<ToolChain*> tcList = ToolChainKitAspect::toolChains(k);
    if (tcList.isEmpty()) {
        result << BuildSystemTask(Task::Warning, ToolChainKitAspect::msgNoToolChainInTarget());
    } else {
        QSet<Abi> targetAbis;
        for (const ToolChain *tc : tcList) {
            targetAbis.insert(tc->targetAbi());
            result << tc->validateKit(k);
        }
        if (targetAbis.count() != 1) {
            result << BuildSystemTask(Task::Error,
                        Tr::tr("Compilers produce code for different ABIs: %1")
                           .arg(Utils::transform<QList>(targetAbis, &Abi::toString).join(", ")));
        }
    }
    return result;
}

void ToolChainKitAspectFactory::fix(Kit *k)
{
    QTC_ASSERT(ToolChainManager::isLoaded(), return);
    const QList<Id> languages = ToolChainManager::allLanguages();
    for (const Id l : languages) {
        const QByteArray tcId = ToolChainKitAspect::toolChainId(k, l);
        if (!tcId.isEmpty() && !ToolChainManager::findToolChain(tcId)) {
            qWarning("Tool chain set up in kit \"%s\" for \"%s\" not found.",
                     qPrintable(k->displayName()),
                     qPrintable(ToolChainManager::displayNameOfLanguageId(l)));
            ToolChainKitAspect::clearToolChain(k, l); // make sure to clear out no longer known tool chains
        }
    }
}

static Id findLanguage(const QString &ls)
{
    QString lsUpper = ls.toUpper();
    return Utils::findOrDefault(ToolChainManager::allLanguages(),
                         [lsUpper](Id l) { return lsUpper == l.toString().toUpper(); });
}

void ToolChainKitAspectFactory::setup(Kit *k)
{
    QTC_ASSERT(ToolChainManager::isLoaded(), return);
    QTC_ASSERT(k, return);

    Store value = storeFromVariant(k->value(id()));
    bool lockToolchains = k->isSdkProvided() && !value.isEmpty();
    if (value.empty())
        value = defaultToolChainValue();

    for (auto i = value.constBegin(); i != value.constEnd(); ++i) {
        Id l = findLanguage(stringFromKey(i.key()));

        if (!l.isValid()) {
            lockToolchains = false;
            continue;
        }

        const QByteArray id = i.value().toByteArray();
        ToolChain *tc = ToolChainManager::findToolChain(id);
        if (tc)
            continue;

        // ID is not found: Might be an ABI string...
        lockToolchains = false;
        const QString abi = QString::fromUtf8(id);
        const Toolchains possibleTcs = ToolChainManager::toolchains([abi, l](const ToolChain *t) {
            return t->targetAbi().toString() == abi && t->language() == l;
        });
        ToolChain *bestTc = nullptr;
        for (ToolChain *tc : possibleTcs) {
            if (!bestTc || tc->priority() > bestTc->priority())
                bestTc = tc;
        }
        if (bestTc)
            ToolChainKitAspect::setToolChain(k, bestTc);
        else
            ToolChainKitAspect::clearToolChain(k, l);
    }

    k->setSticky(id(), lockToolchains);
}

KitAspect *ToolChainKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new Internal::ToolChainKitAspectImpl(k, this);
}

QString ToolChainKitAspectFactory::displayNamePostfix(const Kit *k) const
{
    ToolChain *tc = ToolChainKitAspect::cxxToolChain(k);
    return tc ? tc->displayName() : QString();
}

KitAspectFactory::ItemList ToolChainKitAspectFactory::toUserOutput(const Kit *k) const
{
    ToolChain *tc = ToolChainKitAspect::cxxToolChain(k);
    return {{Tr::tr("Compiler"), tc ? tc->displayName() : Tr::tr("None")}};
}

void ToolChainKitAspectFactory::addToBuildEnvironment(const Kit *k, Environment &env) const
{
    ToolChain *tc = ToolChainKitAspect::cxxToolChain(k);
    if (tc)
        tc->addToEnvironment(env);
}

void ToolChainKitAspectFactory::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    QTC_ASSERT(kit, return);

    // Compatibility with Qt Creator < 4.2:
    expander->registerVariable("Compiler:Name", Tr::tr("Compiler"),
                               [kit] {
                                   const ToolChain *tc = ToolChainKitAspect::cxxToolChain(kit);
                                   return tc ? tc->displayName() : Tr::tr("None");
                               });

    expander->registerVariable("Compiler:Executable", Tr::tr("Path to the compiler executable"),
                               [kit] {
                                   const ToolChain *tc = ToolChainKitAspect::cxxToolChain(kit);
                                   return tc ? tc->compilerCommand().path() : QString();
                               });

    // After 4.2
    expander->registerPrefix("Compiler:Name", Tr::tr("Compiler for different languages"),
                             [kit](const QString &ls) {
                                 const ToolChain *tc = ToolChainKitAspect::toolChain(kit, findLanguage(ls));
                                 return tc ? tc->displayName() : Tr::tr("None");
                             });
    expander->registerPrefix("Compiler:Executable", Tr::tr("Compiler executable for different languages"),
                             [kit](const QString &ls) {
                                 const ToolChain *tc = ToolChainKitAspect::toolChain(kit, findLanguage(ls));
                                 return tc ? tc->compilerCommand().path() : QString();
                             });
}

QList<OutputLineParser *> ToolChainKitAspectFactory::createOutputParsers(const Kit *k) const
{
    for (const Id langId : {Constants::CXX_LANGUAGE_ID, Constants::C_LANGUAGE_ID}) {
        if (const ToolChain * const tc = ToolChainKitAspect::toolChain(k, langId))
            return tc->createOutputParsers();
    }
    return {};
}

QSet<Id> ToolChainKitAspectFactory::availableFeatures(const Kit *k) const
{
    QSet<Id> result;
    for (ToolChain *tc : ToolChainKitAspect::toolChains(k))
        result.insert(tc->typeId().withPrefix("ToolChain."));
    return result;
}

Id ToolChainKitAspect::id()
{
    // "PE.Profile.ToolChain" until 4.2
    // "PE.Profile.ToolChains" temporarily before 4.3 (May 2017)
    return "PE.Profile.ToolChainsV3";
}

QByteArray ToolChainKitAspect::toolChainId(const Kit *k, Id language)
{
    QTC_ASSERT(ToolChainManager::isLoaded(), return nullptr);
    if (!k)
        return {};
    Store value = storeFromVariant(k->value(ToolChainKitAspect::id()));
    return value.value(language.toKey(), QByteArray()).toByteArray();
}

ToolChain *ToolChainKitAspect::toolChain(const Kit *k, Id language)
{
    return ToolChainManager::findToolChain(toolChainId(k, language));
}

ToolChain *ToolChainKitAspect::cToolChain(const Kit *k)
{
    return ToolChainManager::findToolChain(toolChainId(k, ProjectExplorer::Constants::C_LANGUAGE_ID));
}

ToolChain *ToolChainKitAspect::cxxToolChain(const Kit *k)
{
    return ToolChainManager::findToolChain(toolChainId(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID));
}


QList<ToolChain *> ToolChainKitAspect::toolChains(const Kit *k)
{
    QTC_ASSERT(k, return {});

    const Store value = storeFromVariant(k->value(ToolChainKitAspect::id()));
    const QList<ToolChain *> tcList
            = transform<QList>(ToolChainManager::allLanguages(), [&value](Id l) {
                return ToolChainManager::findToolChain(value.value(l.toKey()).toByteArray());
            });
    return filtered(tcList, [](ToolChain *tc) { return tc; });
}

void ToolChainKitAspect::setToolChain(Kit *k, ToolChain *tc)
{
    QTC_ASSERT(tc, return);
    QTC_ASSERT(k, return);
    Store result = storeFromVariant(k->value(ToolChainKitAspect::id()));
    result.insert(tc->language().toKey(), tc->id());

    k->setValue(id(), variantFromStore(result));
}

/**
 * @brief ToolChainKitAspect::setAllToolChainsToMatch
 *
 * Set up all toolchains to be similar to the one toolchain provided. Similar ideally means
 * that all toolchains use the "same" compiler from the same installation, but we will
 * settle for a toolchain with a matching API instead.
 *
 * @param k The kit to set up
 * @param tc The toolchain to match other languages for.
 */
void ToolChainKitAspect::setAllToolChainsToMatch(Kit *k, ToolChain *tc)
{
    QTC_ASSERT(tc, return);
    QTC_ASSERT(k, return);

    const Toolchains allTcList = ToolChainManager::toolchains();
    QTC_ASSERT(allTcList.contains(tc), return);

    Store result = storeFromVariant(k->value(ToolChainKitAspect::id()));
    result.insert(tc->language().toKey(), tc->id());

    for (const Id l : ToolChainManager::allLanguages()) {
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
            result.insert(l.toKey(), bestMatch->id());
        else if (match)
            result.insert(l.toKey(), match->id());
        else
            result.insert(l.toKey(), QByteArray());
    }

    k->setValue(id(), variantFromStore(result));
}

void ToolChainKitAspect::clearToolChain(Kit *k, Id language)
{
    QTC_ASSERT(language.isValid(), return);
    QTC_ASSERT(k, return);

    Store result = storeFromVariant(k->value(ToolChainKitAspect::id()));
    result.insert(language.toKey(), QByteArray());
    k->setValue(id(), variantFromStore(result));
}

Abi ToolChainKitAspect::targetAbi(const Kit *k)
{
    const QList<ToolChain *> tcList = toolChains(k);
    // Find the best possible ABI for all the tool chains...
    Abi cxxAbi;
    QHash<Abi, int> abiCount;
    for (ToolChain *tc : tcList) {
        Abi ta = tc->targetAbi();
        if (tc->language() == Id(Constants::CXX_LANGUAGE_ID))
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

QString ToolChainKitAspect::msgNoToolChainInTarget()
{
    return Tr::tr("No compiler set in kit.");
}

void ToolChainKitAspectFactory::onKitsLoaded()
{
    for (Kit *k : KitManager::kits())
        fix(k);

    connect(ToolChainManager::instance(), &ToolChainManager::toolChainRemoved,
            this, &ToolChainKitAspectFactory::toolChainRemoved);
    connect(ToolChainManager::instance(), &ToolChainManager::toolChainUpdated,
            this, &ToolChainKitAspectFactory::toolChainUpdated);
}

void ToolChainKitAspectFactory::toolChainUpdated(ToolChain *tc)
{
    for (Kit *k : KitManager::kits()) {
        if (ToolChainKitAspect::toolChain(k, tc->language()) == tc)
            notifyAboutUpdate(k);
    }
}

void ToolChainKitAspectFactory::toolChainRemoved(ToolChain *tc)
{
    Q_UNUSED(tc)
    for (Kit *k : KitManager::kits())
        fix(k);
}

const ToolChainKitAspectFactory thsToolChainKitAspectFactory;

// --------------------------------------------------------------------------
// DeviceTypeKitAspect:
// --------------------------------------------------------------------------
namespace Internal {
class DeviceTypeKitAspectImpl final : public KitAspect
{
public:
    DeviceTypeKitAspectImpl(Kit *workingCopy, const KitAspectFactory *factory)
        : KitAspect(workingCopy, factory), m_comboBox(createSubWidget<QComboBox>())
    {
        for (IDeviceFactory *factory : IDeviceFactory::allDeviceFactories())
            m_comboBox->addItem(factory->displayName(), factory->deviceType().toSetting());
        m_comboBox->setToolTip(factory->description());
        refresh();
        connect(m_comboBox, &QComboBox::currentIndexChanged,
                this, &DeviceTypeKitAspectImpl::currentTypeChanged);
    }

    ~DeviceTypeKitAspectImpl() override { delete m_comboBox; }

private:
    void addToLayoutImpl(Layouting::LayoutItem &builder) override
    {
        addMutableAction(m_comboBox);
        builder.addItem(m_comboBox);
    }

    void makeReadOnly() override { m_comboBox->setEnabled(false); }

    void refresh() override
    {
        Id devType = DeviceTypeKitAspect::deviceTypeId(m_kit);
        if (!devType.isValid())
            m_comboBox->setCurrentIndex(-1);
        for (int i = 0; i < m_comboBox->count(); ++i) {
            if (m_comboBox->itemData(i) == devType.toSetting()) {
                m_comboBox->setCurrentIndex(i);
                break;
            }
        }
    }

    void currentTypeChanged(int idx)
    {
        Id type = idx < 0 ? Id() : Id::fromSetting(m_comboBox->itemData(idx));
        DeviceTypeKitAspect::setDeviceTypeId(m_kit, type);
    }

    QComboBox *m_comboBox;
};
} // namespace Internal

class DeviceTypeKitAspectFactory : public KitAspectFactory
{
public:
    DeviceTypeKitAspectFactory();

    void setup(Kit *k) override;
    Tasks validate(const Kit *k) const override;
    KitAspect *createKitAspect(Kit *k) const override;
    ItemList toUserOutput(const Kit *k) const override;

    QSet<Id> supportedPlatforms(const Kit *k) const override;
    QSet<Id> availableFeatures(const Kit *k) const override;
};

DeviceTypeKitAspectFactory::DeviceTypeKitAspectFactory()
{
    setId(DeviceTypeKitAspect::id());
    setDisplayName(Tr::tr("Run device type"));
    setDescription(Tr::tr("The type of device to run applications on."));
    setPriority(33000);
    makeEssential();
}

void DeviceTypeKitAspectFactory::setup(Kit *k)
{
    if (k && !k->hasValue(id()))
        k->setValue(id(), QByteArray(Constants::DESKTOP_DEVICE_TYPE));
}

Tasks DeviceTypeKitAspectFactory::validate(const Kit *k) const
{
    Q_UNUSED(k)
    return {};
}

KitAspect *DeviceTypeKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new Internal::DeviceTypeKitAspectImpl(k, this);
}

KitAspectFactory::ItemList DeviceTypeKitAspectFactory::toUserOutput(const Kit *k) const
{
    QTC_ASSERT(k, return {});
    Id type = DeviceTypeKitAspect::deviceTypeId(k);
    QString typeDisplayName = Tr::tr("Unknown device type");
    if (type.isValid()) {
        if (IDeviceFactory *factory = IDeviceFactory::find(type))
            typeDisplayName = factory->displayName();
    }
    return {{Tr::tr("Device type"), typeDisplayName}};
}

const Id DeviceTypeKitAspect::id()
{
    return "PE.Profile.DeviceType";
}

const Id DeviceTypeKitAspect::deviceTypeId(const Kit *k)
{
    return k ? Id::fromSetting(k->value(DeviceTypeKitAspect::id())) : Id();
}

void DeviceTypeKitAspect::setDeviceTypeId(Kit *k, Id type)
{
    QTC_ASSERT(k, return);
    k->setValue(DeviceTypeKitAspect::id(), type.toSetting());
}

QSet<Id> DeviceTypeKitAspectFactory::supportedPlatforms(const Kit *k) const
{
    return {DeviceTypeKitAspect::deviceTypeId(k)};
}

QSet<Id> DeviceTypeKitAspectFactory::availableFeatures(const Kit *k) const
{
    Id id = DeviceTypeKitAspect::deviceTypeId(k);
    if (id.isValid())
        return {id.withPrefix("DeviceType.")};
    return {};
}

const DeviceTypeKitAspectFactory theDeviceTypeKitAspectFactory;

// --------------------------------------------------------------------------
// DeviceKitAspect:
// --------------------------------------------------------------------------
namespace Internal {
class DeviceKitAspectImpl final : public KitAspect
{
public:
    DeviceKitAspectImpl(Kit *workingCopy, const KitAspectFactory *factory)
        : KitAspect(workingCopy, factory),
        m_comboBox(createSubWidget<QComboBox>()),
        m_model(new DeviceManagerModel(DeviceManager::instance()))
    {
        m_comboBox->setSizePolicy(QSizePolicy::Preferred,
                                  m_comboBox->sizePolicy().verticalPolicy());
        m_comboBox->setModel(m_model);
        m_comboBox->setMinimumContentsLength(16); // Don't stretch too much for Kit Page
        m_manageButton = createManageButton(Constants::DEVICE_SETTINGS_PAGE_ID);
        refresh();
        m_comboBox->setToolTip(factory->description());

        connect(m_model, &QAbstractItemModel::modelAboutToBeReset,
                this, &DeviceKitAspectImpl::modelAboutToReset);
        connect(m_model, &QAbstractItemModel::modelReset,
                this, &DeviceKitAspectImpl::modelReset);
        connect(m_comboBox, &QComboBox::currentIndexChanged,
                this, &DeviceKitAspectImpl::currentDeviceChanged);
    }

    ~DeviceKitAspectImpl() override
    {
        delete m_comboBox;
        delete m_model;
        delete m_manageButton;
    }

private:
    void addToLayoutImpl(Layouting::LayoutItem &builder) override
    {
        addMutableAction(m_comboBox);
        builder.addItem(m_comboBox);
        builder.addItem(m_manageButton);
    }

    void makeReadOnly() override { m_comboBox->setEnabled(false); }

    void refresh() override
    {
        m_model->setTypeFilter(DeviceTypeKitAspect::deviceTypeId(m_kit));
        m_comboBox->setCurrentIndex(m_model->indexOf(DeviceKitAspect::device(m_kit)));
    }

    void modelAboutToReset()
    {
        m_selectedId = m_model->deviceId(m_comboBox->currentIndex());
        m_ignoreChanges.lock();
    }

    void modelReset()
    {
        m_comboBox->setCurrentIndex(m_model->indexForId(m_selectedId));
        m_ignoreChanges.unlock();
    }

    void currentDeviceChanged()
    {
        if (m_ignoreChanges.isLocked())
            return;
        DeviceKitAspect::setDeviceId(m_kit, m_model->deviceId(m_comboBox->currentIndex()));
    }

    Guard m_ignoreChanges;
    QComboBox *m_comboBox;
    QWidget *m_manageButton;
    DeviceManagerModel *m_model;
    Id m_selectedId;
};
} // namespace Internal

class DeviceKitAspectFactory : public KitAspectFactory
{
public:
    DeviceKitAspectFactory();

private:
    Tasks validate(const Kit *k) const override;
    void fix(Kit *k) override;
    void setup(Kit *k) override;

    KitAspect *createKitAspect(Kit *k) const override;

    QString displayNamePostfix(const Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    void addToMacroExpander(Kit *kit, MacroExpander *expander) const override;

    QVariant defaultValue(const Kit *k) const;

    void onKitsLoaded() override;
    void deviceUpdated(Id dataId);
    void devicesChanged();
    void kitUpdated(Kit *k);
};

DeviceKitAspectFactory::DeviceKitAspectFactory()
{
    setId(DeviceKitAspect::id());
    setDisplayName(Tr::tr("Run device"));
    setDescription(Tr::tr("The device to run the applications on."));
    setPriority(32000);
}

QVariant DeviceKitAspectFactory::defaultValue(const Kit *k) const
{
    Id type = DeviceTypeKitAspect::deviceTypeId(k);
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
    return {};
}

Tasks DeviceKitAspectFactory::validate(const Kit *k) const
{
    IDevice::ConstPtr dev = DeviceKitAspect::device(k);
    Tasks result;
    if (dev.isNull())
        result.append(BuildSystemTask(Task::Warning, Tr::tr("No device set.")));
    else if (!dev->isCompatibleWith(k))
        result.append(BuildSystemTask(Task::Error, Tr::tr("Device is incompatible with this kit.")));

    if (dev)
        result.append(dev->validate());

    return result;
}

void DeviceKitAspectFactory::fix(Kit *k)
{
    IDevice::ConstPtr dev = DeviceKitAspect::device(k);
    if (!dev.isNull() && !dev->isCompatibleWith(k)) {
        qWarning("Device is no longer compatible with kit \"%s\", removing it.",
                 qPrintable(k->displayName()));
        DeviceKitAspect::setDeviceId(k, Id());
    }
}

void DeviceKitAspectFactory::setup(Kit *k)
{
    QTC_ASSERT(DeviceManager::instance()->isLoaded(), return);
    IDevice::ConstPtr dev = DeviceKitAspect::device(k);
    if (!dev.isNull() && dev->isCompatibleWith(k))
        return;

    DeviceKitAspect::setDeviceId(k, Id::fromSetting(defaultValue(k)));
}

KitAspect *DeviceKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new Internal::DeviceKitAspectImpl(k, this);
}

QString DeviceKitAspectFactory::displayNamePostfix(const Kit *k) const
{
    IDevice::ConstPtr dev = DeviceKitAspect::device(k);
    return dev.isNull() ? QString() : dev->displayName();
}

KitAspectFactory::ItemList DeviceKitAspectFactory::toUserOutput(const Kit *k) const
{
    IDevice::ConstPtr dev = DeviceKitAspect::device(k);
    return {{Tr::tr("Device"), dev.isNull() ? Tr::tr("Unconfigured") : dev->displayName()}};
}

void DeviceKitAspectFactory::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    QTC_ASSERT(kit, return);
    expander->registerVariable("Device:HostAddress", Tr::tr("Host address"), [kit] {
        const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
        return device ? device->sshParameters().host() : QString();
    });
    expander->registerVariable("Device:SshPort", Tr::tr("SSH port"), [kit] {
        const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
        return device ? QString::number(device->sshParameters().port()) : QString();
    });
    expander->registerVariable("Device:UserName", Tr::tr("User name"), [kit] {
        const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
        return device ? device->sshParameters().userName() : QString();
    });
    expander->registerVariable("Device:KeyFile", Tr::tr("Private key file"), [kit] {
        const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
        return device ? device->sshParameters().privateKeyFile.toString() : QString();
    });
    expander->registerVariable("Device:Name", Tr::tr("Device name"), [kit] {
        const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
        return device ? device->displayName() : QString();
    });
    expander->registerFileVariables("Device::Root", Tr::tr("Device root directory"), [kit] {
        const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
        return device ? device->rootPath() : FilePath{};
    });
}

Id DeviceKitAspect::id()
{
    return "PE.Profile.Device";
}

IDevice::ConstPtr DeviceKitAspect::device(const Kit *k)
{
    QTC_ASSERT(DeviceManager::instance()->isLoaded(), return IDevice::ConstPtr());
    return DeviceManager::instance()->find(deviceId(k));
}

Id DeviceKitAspect::deviceId(const Kit *k)
{
    return k ? Id::fromSetting(k->value(DeviceKitAspect::id())) : Id();
}

void DeviceKitAspect::setDevice(Kit *k, IDevice::ConstPtr dev)
{
    setDeviceId(k, dev ? dev->id() : Id());
}

void DeviceKitAspect::setDeviceId(Kit *k, Id id)
{
    QTC_ASSERT(k, return);
    k->setValue(DeviceKitAspect::id(), id.toSetting());
}

FilePath DeviceKitAspect::deviceFilePath(const Kit *k, const QString &pathOnDevice)
{
    if (IDevice::ConstPtr dev = device(k))
        return dev->filePath(pathOnDevice);
    return FilePath::fromString(pathOnDevice);
}

void DeviceKitAspectFactory::onKitsLoaded()
{
    for (Kit *k : KitManager::kits())
        fix(k);

    DeviceManager *dm = DeviceManager::instance();
    connect(dm, &DeviceManager::deviceListReplaced, this, &DeviceKitAspectFactory::devicesChanged);
    connect(dm, &DeviceManager::deviceAdded, this, &DeviceKitAspectFactory::devicesChanged);
    connect(dm, &DeviceManager::deviceRemoved, this, &DeviceKitAspectFactory::devicesChanged);
    connect(dm, &DeviceManager::deviceUpdated, this, &DeviceKitAspectFactory::deviceUpdated);

    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &DeviceKitAspectFactory::kitUpdated);
    connect(KitManager::instance(), &KitManager::unmanagedKitUpdated,
            this, &DeviceKitAspectFactory::kitUpdated);
}

void DeviceKitAspectFactory::deviceUpdated(Id id)
{
    for (Kit *k : KitManager::kits()) {
        if (DeviceKitAspect::deviceId(k) == id)
            notifyAboutUpdate(k);
    }
}

void DeviceKitAspectFactory::kitUpdated(Kit *k)
{
    setup(k); // Set default device if necessary
}

void DeviceKitAspectFactory::devicesChanged()
{
    for (Kit *k : KitManager::kits())
        setup(k); // Set default device if necessary
}

const DeviceKitAspectFactory theDeviceKitAspectFactory;


// --------------------------------------------------------------------------
// BuildDeviceKitAspect:
// --------------------------------------------------------------------------
namespace Internal {
class BuildDeviceKitAspectImpl final : public KitAspect
{
public:
    BuildDeviceKitAspectImpl(Kit *workingCopy, const KitAspectFactory *factory)
        : KitAspect(workingCopy, factory),
        m_comboBox(createSubWidget<QComboBox>()),
        m_model(new DeviceManagerModel(DeviceManager::instance()))
    {
        m_comboBox->setSizePolicy(QSizePolicy::Ignored, m_comboBox->sizePolicy().verticalPolicy());
        m_comboBox->setModel(m_model);
        m_manageButton = createManageButton(Constants::DEVICE_SETTINGS_PAGE_ID);
        refresh();
        m_comboBox->setToolTip(factory->description());

        connect(m_model, &QAbstractItemModel::modelAboutToBeReset,
                this, &BuildDeviceKitAspectImpl::modelAboutToReset);
        connect(m_model, &QAbstractItemModel::modelReset,
                this, &BuildDeviceKitAspectImpl::modelReset);
        connect(m_comboBox, &QComboBox::currentIndexChanged,
                this, &BuildDeviceKitAspectImpl::currentDeviceChanged);
    }

    ~BuildDeviceKitAspectImpl() override
    {
        delete m_comboBox;
        delete m_model;
        delete m_manageButton;
    }

private:
    void addToLayoutImpl(Layouting::LayoutItem &builder) override
    {
        addMutableAction(m_comboBox);
        builder.addItem(m_comboBox);
        builder.addItem(m_manageButton);
    }

    void makeReadOnly() override { m_comboBox->setEnabled(false); }

    void refresh() override
    {
        QList<Id> blackList;
        const DeviceManager *dm = DeviceManager::instance();
        for (int i = 0; i < dm->deviceCount(); ++i) {
            IDevice::ConstPtr device = dm->deviceAt(i);
            if (!device->usableAsBuildDevice())
                blackList.append(device->id());
        }

        m_model->setFilter(blackList);
        m_comboBox->setCurrentIndex(m_model->indexOf(BuildDeviceKitAspect::device(m_kit)));
    }

    void modelAboutToReset()
    {
        m_selectedId = m_model->deviceId(m_comboBox->currentIndex());
        m_ignoreChanges.lock();
    }

    void modelReset()
    {
        m_comboBox->setCurrentIndex(m_model->indexForId(m_selectedId));
        m_ignoreChanges.unlock();
    }

    void currentDeviceChanged()
    {
        if (m_ignoreChanges.isLocked())
            return;
        BuildDeviceKitAspect::setDeviceId(m_kit, m_model->deviceId(m_comboBox->currentIndex()));
    }

    Guard m_ignoreChanges;
    QComboBox *m_comboBox;
    QWidget *m_manageButton;
    DeviceManagerModel *m_model;
    Id m_selectedId;
};
} // namespace Internal

class BuildDeviceKitAspectFactory : public KitAspectFactory
{
public:
    BuildDeviceKitAspectFactory();

private:
    void setup(Kit *k) override;
    Tasks validate(const Kit *k) const override;

    KitAspect *createKitAspect(Kit *k) const override;

    QString displayNamePostfix(const Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    void addToMacroExpander(Kit *kit, MacroExpander *expander) const override;

    void onKitsLoaded() override;
    void deviceUpdated(Id dataId);
    void devicesChanged();
    void kitUpdated(Kit *k);
};

BuildDeviceKitAspectFactory::BuildDeviceKitAspectFactory()
{
    setId(BuildDeviceKitAspect::id());
    setDisplayName(Tr::tr("Build device"));
    setDescription(Tr::tr("The device used to build applications on."));
    setPriority(31900);
}

static IDeviceConstPtr defaultDevice()
{
    return DeviceManager::defaultDesktopDevice();
}

void BuildDeviceKitAspectFactory::setup(Kit *k)
{
    QTC_ASSERT(DeviceManager::instance()->isLoaded(), return );
    IDevice::ConstPtr dev = BuildDeviceKitAspect::device(k);
    if (!dev.isNull())
        return;

    dev = defaultDevice();
    BuildDeviceKitAspect::setDeviceId(k, dev ? dev->id() : Id());
}

Tasks BuildDeviceKitAspectFactory::validate(const Kit *k) const
{
    IDevice::ConstPtr dev = BuildDeviceKitAspect::device(k);
    Tasks result;
    if (dev.isNull())
        result.append(BuildSystemTask(Task::Warning, Tr::tr("No build device set.")));

    return result;
}

KitAspect *BuildDeviceKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new Internal::BuildDeviceKitAspectImpl(k, this);
}

QString BuildDeviceKitAspectFactory::displayNamePostfix(const Kit *k) const
{
    IDevice::ConstPtr dev = BuildDeviceKitAspect::device(k);
    return dev.isNull() ? QString() : dev->displayName();
}

KitAspectFactory::ItemList BuildDeviceKitAspectFactory::toUserOutput(const Kit *k) const
{
    IDevice::ConstPtr dev = BuildDeviceKitAspect::device(k);
    return {{Tr::tr("Build device"), dev.isNull() ? Tr::tr("Unconfigured") : dev->displayName()}};
}

void BuildDeviceKitAspectFactory::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    QTC_ASSERT(kit, return);
    expander->registerVariable("BuildDevice:HostAddress", Tr::tr("Build host address"), [kit] {
        const IDevice::ConstPtr device = BuildDeviceKitAspect::device(kit);
        return device ? device->sshParameters().host() : QString();
    });
    expander->registerVariable("BuildDevice:SshPort", Tr::tr("Build SSH port"), [kit] {
        const IDevice::ConstPtr device = BuildDeviceKitAspect::device(kit);
        return device ? QString::number(device->sshParameters().port()) : QString();
    });
    expander->registerVariable("BuildDevice:UserName", Tr::tr("Build user name"), [kit] {
        const IDevice::ConstPtr device = BuildDeviceKitAspect::device(kit);
        return device ? device->sshParameters().userName() : QString();
    });
    expander->registerVariable("BuildDevice:KeyFile", Tr::tr("Build private key file"), [kit] {
        const IDevice::ConstPtr device = BuildDeviceKitAspect::device(kit);
        return device ? device->sshParameters().privateKeyFile.toString() : QString();
    });
    expander->registerVariable("BuildDevice:Name", Tr::tr("Build device name"), [kit] {
        const IDevice::ConstPtr device = BuildDeviceKitAspect::device(kit);
        return device ? device->settings()->displayName() : QString();
    });
    expander
        ->registerFileVariables("BuildDevice::Root", Tr::tr("Build device root directory"), [kit] {
            const IDevice::ConstPtr device = BuildDeviceKitAspect::device(kit);
            return device ? device->rootPath() : FilePath{};
        });
}

Id BuildDeviceKitAspect::id()
{
    return "PE.Profile.BuildDevice";
}

IDevice::ConstPtr BuildDeviceKitAspect::device(const Kit *k)
{
    QTC_ASSERT(DeviceManager::instance()->isLoaded(), return IDevice::ConstPtr());
    IDevice::ConstPtr dev = DeviceManager::instance()->find(deviceId(k));
    if (!dev)
        dev = defaultDevice();
    return dev;
}

Id BuildDeviceKitAspect::deviceId(const Kit *k)
{
    return k ? Id::fromSetting(k->value(BuildDeviceKitAspect::id())) : Id();
}

void BuildDeviceKitAspect::setDevice(Kit *k, IDevice::ConstPtr dev)
{
    setDeviceId(k, dev ? dev->id() : Id());
}

void BuildDeviceKitAspect::setDeviceId(Kit *k, Id id)
{
    QTC_ASSERT(k, return);
    k->setValue(BuildDeviceKitAspect::id(), id.toSetting());
}

void BuildDeviceKitAspectFactory::onKitsLoaded()
{
    for (Kit *k : KitManager::kits())
        fix(k);

    DeviceManager *dm = DeviceManager::instance();
    connect(dm, &DeviceManager::deviceListReplaced,
            this, &BuildDeviceKitAspectFactory::devicesChanged);
    connect(dm, &DeviceManager::deviceAdded,
            this, &BuildDeviceKitAspectFactory::devicesChanged);
    connect(dm, &DeviceManager::deviceRemoved,
            this, &BuildDeviceKitAspectFactory::devicesChanged);
    connect(dm, &DeviceManager::deviceUpdated,
            this, &BuildDeviceKitAspectFactory::deviceUpdated);
    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &BuildDeviceKitAspectFactory::kitUpdated);
    connect(KitManager::instance(), &KitManager::unmanagedKitUpdated,
            this, &BuildDeviceKitAspectFactory::kitUpdated);
}

void BuildDeviceKitAspectFactory::deviceUpdated(Id id)
{
    const QList<Kit *> kits = KitManager::kits();
    for (Kit *k : kits) {
        if (BuildDeviceKitAspect::deviceId(k) == id)
            notifyAboutUpdate(k);
    }
}

void BuildDeviceKitAspectFactory::kitUpdated(Kit *k)
{
    setup(k); // Set default device if necessary
}

void BuildDeviceKitAspectFactory::devicesChanged()
{
    const QList<Kit *> kits = KitManager::kits();
    for (Kit *k : kits)
        setup(k); // Set default device if necessary
}

const BuildDeviceKitAspectFactory theBuildDeviceKitAspectFactory;

// --------------------------------------------------------------------------
// EnvironmentKitAspect:
// --------------------------------------------------------------------------
static EnvironmentItem forceMSVCEnglishItem()
{
    static EnvironmentItem item("VSLANG", "1033");
    return item;
}

static bool enforcesMSVCEnglish(const EnvironmentItems &changes)
{
    return changes.contains(forceMSVCEnglishItem());
}

namespace Internal {
class EnvironmentKitAspectImpl final : public KitAspect
{
public:
    EnvironmentKitAspectImpl(Kit *workingCopy, const KitAspectFactory *factory)
        : KitAspect(workingCopy, factory),
          m_summaryLabel(createSubWidget<ElidingLabel>()),
          m_manageButton(createSubWidget<QPushButton>()),
          m_mainWidget(createSubWidget<QWidget>())
    {
        auto *layout = new QVBoxLayout;
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_summaryLabel);
        if (HostOsInfo::isWindowsHost())
            initMSVCOutputSwitch(layout);
        m_mainWidget->setLayout(layout);
        refresh();
        m_manageButton->setText(Tr::tr("Change..."));
        connect(m_manageButton, &QAbstractButton::clicked,
                this, &EnvironmentKitAspectImpl::editEnvironmentChanges);
    }

private:
    void addToLayoutImpl(Layouting::LayoutItem &builder) override
    {
        addMutableAction(m_mainWidget);
        builder.addItem(m_mainWidget);
        builder.addItem(m_manageButton);
    }

    void makeReadOnly() override { m_manageButton->setEnabled(false); }

    void refresh() override
    {
        const EnvironmentItems changes = envWithoutMSVCEnglishEnforcement();
        const QString shortSummary = EnvironmentItem::toStringList(changes).join("; ");
        m_summaryLabel->setText(shortSummary.isEmpty() ? Tr::tr("No changes to apply.") : shortSummary);
    }

    void editEnvironmentChanges()
    {
        MacroExpander *expander = m_kit->macroExpander();
        EnvironmentDialog::Polisher polisher = [expander](QWidget *w) {
            VariableChooser::addSupportForChildWidgets(w, expander);
        };
        auto changes = EnvironmentDialog::getEnvironmentItems(m_summaryLabel,
                                                              envWithoutMSVCEnglishEnforcement(),
                                                              QString(),
                                                              polisher);
        if (!changes)
            return;

        if (HostOsInfo::isWindowsHost()) {
            // re-add what envWithoutMSVCEnglishEnforcement removed
            // or update vslang checkbox if user added it manually
            if (m_vslangCheckbox->isChecked() && !enforcesMSVCEnglish(*changes))
                changes->append(forceMSVCEnglishItem());
            else if (enforcesMSVCEnglish(*changes))
                m_vslangCheckbox->setChecked(true);
        }
        EnvironmentKitAspect::setEnvironmentChanges(m_kit, *changes);
    }

    EnvironmentItems envWithoutMSVCEnglishEnforcement() const
    {
        EnvironmentItems changes = EnvironmentKitAspect::environmentChanges(m_kit);

        if (HostOsInfo::isWindowsHost())
            changes.removeAll(forceMSVCEnglishItem());

        return sorted(std::move(changes), [](const EnvironmentItem &lhs, const EnvironmentItem &rhs)
        { return QString::localeAwareCompare(lhs.name, rhs.name) < 0; });
    }

    void initMSVCOutputSwitch(QVBoxLayout *layout)
    {
        m_vslangCheckbox = new QCheckBox(Tr::tr("Force UTF-8 MSVC compiler output"));
        layout->addWidget(m_vslangCheckbox);
        m_vslangCheckbox->setToolTip(Tr::tr("Either switches MSVC to English or keeps the language and "
                                        "just forces UTF-8 output (may vary depending on the used MSVC "
                                        "compiler)."));
        if (enforcesMSVCEnglish(EnvironmentKitAspect::environmentChanges(m_kit)))
            m_vslangCheckbox->setChecked(true);
        connect(m_vslangCheckbox, &QCheckBox::clicked, this, [this](bool checked) {
            EnvironmentItems changes = EnvironmentKitAspect::environmentChanges(m_kit);
            if (!checked && changes.indexOf(forceMSVCEnglishItem()) >= 0)
                changes.removeAll(forceMSVCEnglishItem());
            if (checked && changes.indexOf(forceMSVCEnglishItem()) < 0)
                changes.append(forceMSVCEnglishItem());
            EnvironmentKitAspect::setEnvironmentChanges(m_kit, changes);
        });
    }

    ElidingLabel *m_summaryLabel;
    QPushButton *m_manageButton;
    QCheckBox *m_vslangCheckbox;
    QWidget *m_mainWidget;
};
} // namespace Internal

class EnvironmentKitAspectFactory : public KitAspectFactory
{
public:
    EnvironmentKitAspectFactory();

    Tasks validate(const Kit *k) const override;
    void fix(Kit *k) override;

    void addToBuildEnvironment(const Kit *k, Environment &env) const override;
    void addToRunEnvironment(const Kit *, Environment &) const override;

    KitAspect *createKitAspect(Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;
};

EnvironmentKitAspectFactory::EnvironmentKitAspectFactory()
{
    setId(EnvironmentKitAspect::id());
    setDisplayName(Tr::tr("Environment"));
    setDescription(Tr::tr("Additional build environment settings when using this kit."));
    setPriority(29000);
}

Tasks EnvironmentKitAspectFactory::validate(const Kit *k) const
{
    Tasks result;
    QTC_ASSERT(k, return result);

    const QVariant variant = k->value(EnvironmentKitAspect::id());
    if (!variant.isNull() && !variant.canConvert(QVariant::List))
        result << BuildSystemTask(Task::Error, Tr::tr("The environment setting value is invalid."));

    return result;
}

void EnvironmentKitAspectFactory::fix(Kit *k)
{
    QTC_ASSERT(k, return);

    const QVariant variant = k->value(EnvironmentKitAspect::id());
    if (!variant.isNull() && !variant.canConvert(QVariant::List)) {
        qWarning("Kit \"%s\" has a wrong environment value set.", qPrintable(k->displayName()));
        EnvironmentKitAspect::setEnvironmentChanges(k, EnvironmentItems());
    }
}

void EnvironmentKitAspectFactory::addToBuildEnvironment(const Kit *k, Environment &env) const
{
    const QStringList values
            = transform(EnvironmentItem::toStringList(EnvironmentKitAspect::environmentChanges(k)),
                               [k](const QString &v) { return k->macroExpander()->expand(v); });
    env.modify(EnvironmentItem::fromStringList(values));
}

void EnvironmentKitAspectFactory::addToRunEnvironment(const Kit *k, Environment &env) const
{
    addToBuildEnvironment(k, env);
}

KitAspect *EnvironmentKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new Internal::EnvironmentKitAspectImpl(k, this);
}

KitAspectFactory::ItemList EnvironmentKitAspectFactory::toUserOutput(const Kit *k) const
{
    return {{Tr::tr("Environment"),
         EnvironmentItem::toStringList(EnvironmentKitAspect::environmentChanges(k)).join("<br>")}};
}

Id EnvironmentKitAspect::id()
{
    return "PE.Profile.Environment";
}

EnvironmentItems EnvironmentKitAspect::environmentChanges(const Kit *k)
{
     if (k)
         return EnvironmentItem::fromStringList(k->value(EnvironmentKitAspect::id()).toStringList());
     return {};
}

void EnvironmentKitAspect::setEnvironmentChanges(Kit *k, const EnvironmentItems &changes)
{
    if (k)
        k->setValue(EnvironmentKitAspect::id(), EnvironmentItem::toStringList(changes));
}

const EnvironmentKitAspectFactory theEnvironmentKitAspectFactory;

} // namespace ProjectExplorer
