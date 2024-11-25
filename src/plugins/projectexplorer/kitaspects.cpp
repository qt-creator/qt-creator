// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "kitaspects.h"

#include "projectexplorertr.h"
#include "kit.h"
#include "kitaspect.h"
#include "kitmanager.h"
#include "toolchain.h"

#include <utils/algorithm.h>
#include <utils/elidinglabel.h>
#include <utils/environment.h>
#include <utils/environmentdialog.h>
#include <utils/guard.h>
#include <utils/guiutils.h>
#include <utils/layoutbuilder.h>
#include <utils/listmodel.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/variablechooser.h>

#include <QCheckBox>
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

    void addToInnerLayout(Layouting::Layout &builder) override
    {
        addMutableAction(m_chooser);
        builder.addItem(Layouting::Span(2, m_chooser));
    }

    void refresh() override
    {
        if (!m_ignoreChanges.isLocked())
            m_chooser->setFilePath(SysRootKitAspect::sysRoot(kit()));
    }

    void pathWasChanged()
    {
        const GuardLocker locker(m_ignoreChanges);
        SysRootKitAspect::setSysRoot(kit(), m_chooser->filePath());
    }

    PathChooser *m_chooser;
    Guard m_ignoreChanges;
};

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

const SysRootKitAspectFactory theSyRootKitAspectFactory;

} // namespace Internal

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

    for (Toolchain *tc : ToolchainKitAspect::toolChains(k)) {
        if (!tc->sysRoot().isEmpty())
            return FilePath::fromString(tc->sysRoot());
    }
    return {};
}

void SysRootKitAspect::setSysRoot(Kit *k, const FilePath &v)
{
    if (!k)
        return;

    for (Toolchain *tc : ToolchainKitAspect::toolChains(k)) {
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
    void addToInnerLayout(Layouting::Layout &builder) override
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
        MacroExpander *expander = kit()->macroExpander();
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
        EnvironmentKitAspect::setEnvironmentChanges(kit(), *changes);
    }

    EnvironmentItems envWithoutMSVCEnglishEnforcement() const
    {
        EnvironmentItems changes = EnvironmentKitAspect::environmentChanges(kit());

        if (HostOsInfo::isWindowsHost())
            changes.removeAll(forceMSVCEnglishItem());

        return changes;
    }

    void initMSVCOutputSwitch(QVBoxLayout *layout)
    {
        m_vslangCheckbox = new QCheckBox(Tr::tr("Force UTF-8 MSVC compiler output"));
        layout->addWidget(m_vslangCheckbox);
        m_vslangCheckbox->setToolTip(Tr::tr("Either switches MSVC to English or keeps the language and "
                                        "just forces UTF-8 output (may vary depending on the used MSVC "
                                        "compiler)."));
        if (enforcesMSVCEnglish(EnvironmentKitAspect::environmentChanges(kit())))
            m_vslangCheckbox->setChecked(true);
        connect(m_vslangCheckbox, &QCheckBox::clicked, this, [this](bool checked) {
            EnvironmentItems changes = EnvironmentKitAspect::environmentChanges(kit());
            if (!checked && changes.indexOf(forceMSVCEnglishItem()) >= 0)
                changes.removeAll(forceMSVCEnglishItem());
            if (checked && changes.indexOf(forceMSVCEnglishItem()) < 0)
                changes.append(forceMSVCEnglishItem());
            EnvironmentKitAspect::setEnvironmentChanges(kit(), changes);
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
    if (!variant.isNull() && !variant.canConvert(QMetaType(QMetaType::QVariantList)))
        result << BuildSystemTask(Task::Error, Tr::tr("The environment setting value is invalid."));

    return result;
}

void EnvironmentKitAspectFactory::fix(Kit *k)
{
    QTC_ASSERT(k, return);

    const QVariant variant = k->value(EnvironmentKitAspect::id());
    if (!variant.isNull() && !variant.canConvert(QMetaType(QMetaType::QVariantList))) {
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
