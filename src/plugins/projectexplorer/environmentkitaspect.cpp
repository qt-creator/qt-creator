// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "environmentkitaspect.h"

#include "projectexplorertr.h"
#include "kit.h"
#include "kitaspect.h"
#include "kitmanager.h"
#include "toolchain.h"

#include <utils/algorithm.h>
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
#include <QHBoxLayout>
#include <QPushButton>

using namespace Utils;

namespace ProjectExplorer {

static EnvironmentItem forceMSVCEnglishItem()
{
    static EnvironmentItem item("VSLANG", "1033");
    return item;
}

static bool enforcesMSVCEnglish(const EnvironmentItems &changes)
{
    return changes.contains(forceMSVCEnglishItem());
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
        refresh();
        m_buildEnvButton->setText(Tr::tr("Edit Build Environment..."));
        m_buildEnvButton->setIcon({});
        m_runEnvButton->setText(Tr::tr("Edit Run Environment..."));
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
        const auto toString = [](const EnvironmentItems &changes) {
            return EnvironmentItem::toStringList(changes).join("\n");
        };
        m_buildEnvButton->setToolTip(toString(EnvironmentKitAspect::buildEnvChanges(kit())));
        m_runEnvButton->setToolTip(toString(EnvironmentKitAspect::runEnvChanges(kit())));

        // TODO: Set an icon on the button representing whether there are changes or not.
    }

    void editBuildEnvironmentChanges()
    {
        auto changes = EnvironmentDialog::getEnvironmentItems(
            m_mainWidget,
            EnvironmentKitAspect::buildEnvChanges(kit()),
            QString(),
            polisher(),
            Tr::tr("Edit Build Environment"));
        if (!changes)
            return;

        if (m_vslangCheckbox) {
            // re-add what envWithoutMSVCEnglishEnforcement removed
            // or update vslang checkbox if user added it manually
            if (m_vslangCheckbox->isChecked() && !enforcesMSVCEnglish(*changes))
                changes->append(forceMSVCEnglishItem());
            else if (enforcesMSVCEnglish(*changes))
                m_vslangCheckbox->setChecked(true);
        }
        EnvironmentKitAspect::setBuildEnvChanges(kit(), *changes);
    }

    void editRunEnvironmentChanges()
    {
        if (const auto changes = EnvironmentDialog::getEnvironmentItems(
                m_mainWidget,
                EnvironmentKitAspect::runEnvChanges(kit()),
                QString(),
                polisher(),
                Tr::tr("Edit Run Environment"))) {
            EnvironmentKitAspect::setRunEnvChanges(kit(), *changes);
        }
    }

    EnvironmentDialog::Polisher polisher() const
    {
        return [expander = kit()->macroExpander()](QWidget *w) {
            VariableChooser::addSupportForChildWidgets(w, expander);
        };
    }

    void initMSVCOutputSwitch()
    {
        m_vslangCheckbox = new QCheckBox(Tr::tr("Force UTF-8 MSVC output"));
        m_vslangCheckbox->setToolTip(Tr::tr("Either switches MSVC to English or keeps the language and "
                                        "just forces UTF-8 output (may vary depending on the used MSVC "
                                        "compiler)."));
        m_vslangCheckbox->setChecked(
            enforcesMSVCEnglish(EnvironmentKitAspect::buildEnvChanges(kit())));
        connect(m_vslangCheckbox, &QCheckBox::clicked, this, [this](bool checked) {
            EnvironmentItems changes = EnvironmentKitAspect::buildEnvChanges(kit());
            const bool hasVsLangEntry = enforcesMSVCEnglish(changes);
            if (checked && !hasVsLangEntry)
                changes.append(forceMSVCEnglishItem());
            else if (!checked && hasVsLangEntry)
                changes.removeAll(forceMSVCEnglishItem());
            EnvironmentKitAspect::setBuildEnvChanges(kit(), changes);
        });
    }

    QWidget * const m_mainWidget;
    QPushButton * const m_buildEnvButton;
    QPushButton * const m_runEnvButton;
    QCheckBox *m_vslangCheckbox = nullptr;
};

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

    const QVariant variant = k->value(buildEnvId());
    if (!variant.isNull() && !variant.canConvert(QMetaType(QMetaType::QVariantList)))
        result << BuildSystemTask(Task::Error, Tr::tr("The environment setting value is invalid."));

    return result;
}

void EnvironmentKitAspectFactory::fix(Kit *k)
{
    QTC_ASSERT(k, return);

    if (const QVariant variant = k->value(buildEnvId());
        !variant.isNull() && !variant.canConvert(QMetaType(QMetaType::QVariantList))) {
        qWarning("Kit \"%s\" has a wrong build environment value set.", qPrintable(k->displayName()));
        EnvironmentKitAspect::setBuildEnvChanges(k, EnvironmentItems());
    }
    if (const QVariant variant = k->value(runEnvId());
        !variant.isNull() && !variant.canConvert(QMetaType(QMetaType::QVariantList))) {
        qWarning("Kit \"%s\" has a wrong run environment value set.", qPrintable(k->displayName()));
        EnvironmentKitAspect::setRunEnvChanges(k, EnvironmentItems());
    }
}

void EnvironmentKitAspectFactory::addToBuildEnvironment(const Kit *k, Environment &env) const
{
    const QStringList values
        = transform(EnvironmentItem::toStringList(EnvironmentKitAspect::buildEnvChanges(k)),
                    [k](const QString &v) { return k->macroExpander()->expand(v); });
    env.modify(EnvironmentItem::fromStringList(values));
}

void EnvironmentKitAspectFactory::addToRunEnvironment(const Kit *k, Environment &env) const
{
    const QStringList values
        = transform(EnvironmentItem::toStringList(EnvironmentKitAspect::runEnvChanges(k)),
                    [k](const QString &v) { return k->macroExpander()->expand(v); });
    env.modify(EnvironmentItem::fromStringList(values));
}

KitAspect *EnvironmentKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new Internal::EnvironmentKitAspectImpl(k, this);
}

KitAspectFactory::ItemList EnvironmentKitAspectFactory::toUserOutput(const Kit *k) const
{
    ItemList list;
    const auto addIfNotEmpty = [&](const QString &displayName, const EnvironmentItems &changes) {
        if (!changes.isEmpty())
            list.emplaceBack(displayName, EnvironmentItem::toStringList(changes).join("<br>"));
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

EnvironmentItems EnvironmentKitAspect::buildEnvChanges(const Kit *k)
{
     if (k)
         return EnvironmentItem::fromStringList(k->value(buildEnvId()).toStringList());
     return {};
}

void EnvironmentKitAspect::setBuildEnvChanges(Kit *k, const EnvironmentItems &changes)
{
    if (k)
        k->setValue(buildEnvId(), EnvironmentItem::toStringList(changes));
}

EnvironmentItems EnvironmentKitAspect::runEnvChanges(const Kit *k)
{
    if (k)
        return EnvironmentItem::fromStringList(k->value(runEnvId()).toStringList());
    return {};
}

void EnvironmentKitAspect::setRunEnvChanges(Kit *k, const Utils::EnvironmentItems &changes)
{
    if (k)
        k->setValue(runEnvId(), EnvironmentItem::toStringList(changes));
}

} // namespace ProjectExplorer
