// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "environmentkitaspect.h"

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
    void addToInnerLayout(Layouting::Layout &layout) override
    {
        addMutableAction(m_mainWidget);
        layout.addItem(m_mainWidget);
        layout.addItem(m_manageButton);
    }

    void makeReadOnly() override { m_manageButton->setEnabled(false); }

    void refresh() override
    {
        const EnvironmentItems changes = EnvironmentKitAspect::environmentChanges(kit());
        const QString shortSummary = EnvironmentItem::toStringList(changes).join("; ");
        m_summaryLabel->setText(shortSummary.isEmpty() ? Tr::tr("No changes to apply.") : shortSummary);
    }

    void editEnvironmentChanges()
    {
        MacroExpander *expander = kit()->macroExpander();
        EnvironmentDialog::Polisher polisher = [expander](QWidget *w) {
            VariableChooser::addSupportForChildWidgets(w, expander);
        };
        auto changes = EnvironmentDialog::getEnvironmentItems(
            m_summaryLabel, EnvironmentKitAspect::environmentChanges(kit()), QString(), polisher);
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

    void initMSVCOutputSwitch(QVBoxLayout *layout)
    {
        m_vslangCheckbox = new QCheckBox(Tr::tr("Force UTF-8 MSVC compiler output"));
        layout->addWidget(m_vslangCheckbox);
        m_vslangCheckbox->setToolTip(Tr::tr("Either switches MSVC to English or keeps the language and "
                                        "just forces UTF-8 output (may vary depending on the used MSVC "
                                        "compiler)."));
        m_vslangCheckbox->setChecked(
            enforcesMSVCEnglish(EnvironmentKitAspect::environmentChanges(kit())));
        connect(m_vslangCheckbox, &QCheckBox::clicked, this, [this](bool checked) {
            EnvironmentItems changes = EnvironmentKitAspect::environmentChanges(kit());
            const bool hasVsLangEntry = enforcesMSVCEnglish(changes);
            if (checked && !hasVsLangEntry)
                changes.append(forceMSVCEnglishItem());
            else if (!checked && hasVsLangEntry)
                changes.removeAll(forceMSVCEnglishItem());
            EnvironmentKitAspect::setEnvironmentChanges(kit(), changes);
        });
    }

    ElidingLabel *m_summaryLabel;
    QPushButton *m_manageButton;
    QCheckBox *m_vslangCheckbox;
    QWidget *m_mainWidget;
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

const EnvironmentKitAspectFactory theEnvironmentKitAspectFactory;

} // namespace Internal

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

} // namespace ProjectExplorer
