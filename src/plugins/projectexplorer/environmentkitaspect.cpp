// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "environmentkitaspect.h"

#include "devicesupport/devicekitaspects.h"
#include "devicesupport/idevice.h"
#include "projectexplorertr.h"
#include "kit.h"
#include "kitaspect.h"
#include "kitmanager.h"
#include "toolchain.h"

#include <utils/environment.h>
#include <utils/environmentdialog.h>
#include <utils/guiutils.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/variablechooser.h>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QPushButton>

using namespace Utils;

namespace ProjectExplorer {

static EnvironmentItem forceMSVCEnglishItem()
{
    static EnvironmentItem item("VSLANG", "1033");
    return item;
}

static bool enforcesMSVCEnglish(const EnvironmentChanges &changes)
{
    return changes.itemsFromUser().contains(forceMSVCEnglishItem())
        || changes.itemsFromFile().contains(forceMSVCEnglishItem());
}

static Id buildEnvId() { return "PE.Profile.Environment"; }
static Id runEnvId() { return "PE.Profile.RunEnvironment"; }

namespace Internal {
class EnvironmentKitAspectImpl final : public KitAspect
{
public:
    EnvironmentKitAspectImpl(Kit *workingCopy, const KitAspectFactory *factory)
        : KitAspect(workingCopy, factory),
          m_mainWidget(createSubWidget<QWidget>()),
          m_buildEnvButton(createSubWidget<QPushButton>()),
          m_runEnvButton(createSubWidget<QPushButton>())
    {
        addMutableAction(m_mainWidget);
        if (HostOsInfo::isWindowsHost())
            initMSVCOutputSwitch();

        valueToVolatileValue();
        refresh();

        m_buildEnvButton->setText(Tr::tr("Edit Build Environment..."));
        m_buildEnvButton->setIcon({});
        setIgnoreForDirtyHook(m_buildEnvButton);

        m_runEnvButton->setText(Tr::tr("Edit Run Environment..."));
        setIgnoreForDirtyHook(m_runEnvButton);

        connect(m_buildEnvButton, &QAbstractButton::clicked,
                this, &EnvironmentKitAspectImpl::editBuildEnvironmentChanges);
        connect(m_runEnvButton, &QAbstractButton::clicked,
                this, &EnvironmentKitAspectImpl::editRunEnvironmentChanges);
    }

private:
    void addToInnerLayout(Layouting::Layout &layout) override
    {
        Layouting::Layout box(new QHBoxLayout);
        box.setContentsMargins(0, 0, 0, 0);
        box.attachTo(m_mainWidget);
        if (m_vslangCheckbox)
            box.addItem(m_vslangCheckbox);
        box.addItems({m_buildEnvButton, m_runEnvButton});
        box.addItem(Layouting::Stretch(1));
        layout.addItem(m_mainWidget);
    }

    void makeReadOnly() override
    {
        if (m_vslangCheckbox)
            m_vslangCheckbox->setEnabled(false);
        m_buildEnvButton->setEnabled(false);
        m_runEnvButton->setEnabled(false);
    }

    void refresh() override
    {
        m_buildEnvButton->setToolTip(m_buildEnvChanges.toShortSummary(kit()->macroExpander(), true));
        m_runEnvButton->setToolTip(m_runEnvChanges.toShortSummary(kit()->macroExpander(), true));

        // TODO: Set an icon on the button representing whether there are changes or not.
    }

    bool isDirty() const final
    {
        return m_buildEnvChanges != EnvironmentKitAspect::buildEnvChanges(kit())
            || m_runEnvChanges != EnvironmentKitAspect::runEnvChanges(kit());
    }

    bool valueToVolatileValue() final
    {
        bool res1 = updateStorage(m_buildEnvChanges, EnvironmentKitAspect::buildEnvChanges(kit()));
        bool res2 = updateStorage(m_runEnvChanges, EnvironmentKitAspect::runEnvChanges(kit()));
        return res1 || res2;
    }

    bool volatileValueToValue() final
    {
        bool changed = false;
        if (m_buildEnvChanges != EnvironmentKitAspect::buildEnvChanges(kit())) {
            EnvironmentKitAspect::setBuildEnvChanges(kit(), m_buildEnvChanges);
            changed = true;
        }
        if (m_runEnvChanges != EnvironmentKitAspect::runEnvChanges(kit())) {
            EnvironmentKitAspect::setRunEnvChanges(kit(), m_runEnvChanges);
            changed = true;
        }
        return changed;
    }

    void editBuildEnvironmentChanges()
    {
        FilePath browseHint;
        if (const IDeviceConstPtr &device = BuildDeviceKitAspect::device(kit()))
            browseHint = device->rootPath();
        std::optional<EnvironmentChanges> changes = runEnvironmentItemsDialog(
            m_mainWidget,
            m_buildEnvChanges,
            QString(),
            polisher(),
            Tr::tr("Edit Build Environment"),
            browseHint);
        if (!changes)
            return;

        if (m_vslangCheckbox) {
            // re-add what envWithoutMSVCEnglishEnforcement removed
            // or update vslang checkbox if user added it manually
            if (m_vslangCheckbox->isChecked() && !enforcesMSVCEnglish(*changes))
                changes->appendUserItem(forceMSVCEnglishItem());
            else if (enforcesMSVCEnglish(*changes))
                m_vslangCheckbox->setChecked(true);
        }

        if (updateStorage(m_buildEnvChanges, *changes)) {
            emit volatileValueChanged();
            refresh();
            markSettingsDirty();
        }
    }

    void editRunEnvironmentChanges()
    {
        FilePath browseHint;
        if (const IDeviceConstPtr &device = RunDeviceKitAspect::device(kit()))
            browseHint = device->rootPath();
        const std::optional<EnvironmentChanges> changes = runEnvironmentItemsDialog(
            m_mainWidget,
            m_runEnvChanges,
            QString(),
            polisher(),
            Tr::tr("Edit Run Environment"),
            browseHint);
        if (!changes)
            return;

        if (updateStorage(m_runEnvChanges, *changes)) {
            emit volatileValueChanged();
            refresh();
            markSettingsDirty();
        }
    }

    NameValuesDialog::Polisher polisher() const
    {
        return [expander = kit()->macroExpander()](QWidget *w) {
            VariableChooser::addSupportForChildWidgets(w, {w, expander}); // FIXME: Use better guard?
        };
    }

    void initMSVCOutputSwitch()
    {
        m_vslangCheckbox = new QCheckBox(Tr::tr("Force UTF-8 MSVC output"));
        m_vslangCheckbox->setToolTip(Tr::tr("Either switches MSVC to English or keeps the language and "
                                        "just forces UTF-8 output (may vary depending on the used MSVC "
                                        "compiler)."));
        m_vslangCheckbox->setChecked(enforcesMSVCEnglish(m_buildEnvChanges));
        connect(m_vslangCheckbox, &QCheckBox::clicked, this, [this](bool checked) {
            const bool hasVsLangEntry = enforcesMSVCEnglish(m_buildEnvChanges);
            if (checked && !hasVsLangEntry)
                m_buildEnvChanges.appendUserItem(forceMSVCEnglishItem());
            else if (!checked && hasVsLangEntry)
                m_buildEnvChanges.removeUserItem(forceMSVCEnglishItem());
        });
    }

    QWidget * const m_mainWidget;
    QPushButton * const m_buildEnvButton;
    QPushButton * const m_runEnvButton;
    QCheckBox *m_vslangCheckbox = nullptr;

    // Used as "volatile value" of the aspect.
    EnvironmentChanges m_runEnvChanges;
    EnvironmentChanges m_buildEnvChanges;
};

class EnvironmentKitAspectFactory : public KitAspectFactory
{
public:
    EnvironmentKitAspectFactory();

    Tasks validate(const Kit *) const override { return {}; }
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

void EnvironmentKitAspectFactory::addToBuildEnvironment(const Kit *k, Environment &env) const
{
    EnvironmentKitAspect::buildEnvChanges(k).modifyEnvironment(env, k->macroExpander());
}

void EnvironmentKitAspectFactory::addToRunEnvironment(const Kit *k, Environment &env) const
{
    EnvironmentKitAspect::runEnvChanges(k).modifyEnvironment(env, k->macroExpander());
}

KitAspect *EnvironmentKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new Internal::EnvironmentKitAspectImpl(k, this);
}

KitAspectFactory::ItemList EnvironmentKitAspectFactory::toUserOutput(const Kit *k) const
{
    ItemList list;
    const auto addIfNotEmpty = [&](const QString &displayName,
                                   const EnvironmentChanges &changes) {
        if (changes.hasItems()) {
            Environment env;
            changes.modifyEnvironment(env, k->macroExpander());
            list.emplaceBack(displayName, env.toStringList().join("<br>"));
        }
    };
    addIfNotEmpty(Tr::tr("Build Environment"), EnvironmentKitAspect::buildEnvChanges(k));
    addIfNotEmpty(Tr::tr("Run Environment"), EnvironmentKitAspect::runEnvChanges(k));
    return list;
}

const EnvironmentKitAspectFactory theEnvironmentKitAspectFactory;

} // namespace Internal

Id EnvironmentKitAspect::id()
{
    return buildEnvId();
}

EnvironmentChanges EnvironmentKitAspect::buildEnvChanges(const Kit *k)
{
    if (k)
        return EnvironmentChanges::createFromVariant(k->value(buildEnvId()));
    return {};
}

void EnvironmentKitAspect::setBuildEnvChanges(Kit *k, const EnvironmentChanges &changes)
{
    if (k)
        k->setValue(buildEnvId(), changes.toVariant());
}

EnvironmentChanges EnvironmentKitAspect::runEnvChanges(const Kit *k)
{
    if (k)
        return EnvironmentChanges::createFromVariant(k->value(runEnvId()));
    return {};
}

void EnvironmentKitAspect::setRunEnvChanges(Kit *k, const EnvironmentChanges &changes)
{
    if (k)
        k->setValue(runEnvId(), changes.toVariant());
}

} // namespace ProjectExplorer
