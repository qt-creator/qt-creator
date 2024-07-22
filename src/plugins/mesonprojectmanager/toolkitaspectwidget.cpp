// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolkitaspectwidget.h"

#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"
#include "mesontools.h"

#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QCoreApplication>

using namespace ProjectExplorer;
using namespace Utils;

namespace MesonProjectManager::Internal {

// Meson/Ninja KitAspect base

class MesonToolKitAspectImpl final : public KitAspect
{
public:
    MesonToolKitAspectImpl(Kit *kit,
                           const KitAspectFactory *factory,
                           ToolType type);
    ~MesonToolKitAspectImpl() { delete m_toolsComboBox; }

private:
    void addTool(const MesonTools::Tool_t &tool);
    void removeTool(const MesonTools::Tool_t &tool);
    void setCurrentToolIndex(int index);
    int indexOf(const Id &id);
    bool isCompatible(const MesonTools::Tool_t &tool);
    void loadTools();
    void setToDefault();

    void makeReadOnly() final { m_toolsComboBox->setEnabled(false); }

    void addToInnerLayout(Layouting::Layout &parent) final
    {
        addMutableAction(m_toolsComboBox);
        parent.addItem(m_toolsComboBox);
    }

    void refresh() final
    {
        const auto id = [this] {
            if (m_type == ToolType::Meson)
                return MesonToolKitAspect::mesonToolId(m_kit);
            return NinjaToolKitAspect::ninjaToolId(m_kit);
        }();
        m_toolsComboBox->setCurrentIndex(indexOf(id));
    }

    QComboBox *m_toolsComboBox;
    ToolType m_type;
};

MesonToolKitAspectImpl::MesonToolKitAspectImpl(Kit *kit,
                                               const KitAspectFactory *factory,
                                               ToolType type)
    : KitAspect(kit, factory)
    , m_toolsComboBox(createSubWidget<QComboBox>())
    , m_type{type}
{
    setManagingPage(Constants::SettingsPage::TOOLS_ID);

    m_toolsComboBox->setSizePolicy(QSizePolicy::Ignored,
                                   m_toolsComboBox->sizePolicy().verticalPolicy());
    m_toolsComboBox->setEnabled(false);
    m_toolsComboBox->setToolTip(factory->description());
    loadTools();

    connect(MesonTools::instance(), &MesonTools::toolAdded,
            this, &MesonToolKitAspectImpl::addTool);
    connect(MesonTools::instance(), &MesonTools::toolRemoved,
            this, &MesonToolKitAspectImpl::removeTool);
    connect(m_toolsComboBox, &QComboBox::currentIndexChanged,
            this, &MesonToolKitAspectImpl::setCurrentToolIndex);
}



void MesonToolKitAspectImpl::addTool(const MesonTools::Tool_t &tool)
{
    QTC_ASSERT(tool, return );
    if (isCompatible(tool))
        this->m_toolsComboBox->addItem(tool->name(), tool->id().toSetting());
}

void MesonToolKitAspectImpl::removeTool(const MesonTools::Tool_t &tool)
{
    QTC_ASSERT(tool, return );
    if (!isCompatible(tool))
        return;
    const int index = indexOf(tool->id());
    QTC_ASSERT(index >= 0, return );
    if (index == m_toolsComboBox->currentIndex())
        setToDefault();
    m_toolsComboBox->removeItem(index);
}

void MesonToolKitAspectImpl::setCurrentToolIndex(int index)
{
    if (m_toolsComboBox->count() == 0)
        return;
    const Id id = Id::fromSetting(m_toolsComboBox->itemData(index));
    if (m_type == ToolType::Meson)
        MesonToolKitAspect::setMesonTool(m_kit, id);
    else
        NinjaToolKitAspect::setNinjaTool(m_kit, id);
}

int MesonToolKitAspectImpl::indexOf(const Id &id)
{
    for (int i = 0; i < m_toolsComboBox->count(); ++i) {
        if (id == Id::fromSetting(m_toolsComboBox->itemData(i)))
            return i;
    }
    return -1;
}

bool MesonToolKitAspectImpl::isCompatible(const MesonTools::Tool_t &tool)
{
    return (m_type == ToolType::Meson && MesonTools::isMesonWrapper(tool))
           || (m_type == ToolType::Ninja && MesonTools::isNinjaWrapper(tool));
}

void MesonToolKitAspectImpl::loadTools()
{
    for (const MesonTools::Tool_t &tool : MesonTools::tools()) {
        addTool(tool);
    }
    refresh();
    m_toolsComboBox->setEnabled(m_toolsComboBox->count());
}

void MesonToolKitAspectImpl::setToDefault()
{
    const MesonTools::Tool_t autoDetected = [this] {
        if (m_type == ToolType::Meson)
            return MesonTools::autoDetectedMeson();
        return MesonTools::autoDetectedNinja();
    }();

    if (autoDetected) {
        const auto index = indexOf(autoDetected->id());
        m_toolsComboBox->setCurrentIndex(index);
        setCurrentToolIndex(index);
    } else {
        m_toolsComboBox->setCurrentIndex(0);
        setCurrentToolIndex(0);
    }
}

// MesonToolKitAspect

const char MESON_TOOL_ID[] = "MesonProjectManager.MesonKitInformation.Meson";

void MesonToolKitAspect::setMesonTool(Kit *kit, Id id)
{
    QTC_ASSERT(kit && id.isValid(), return );
    kit->setValue(MESON_TOOL_ID, id.toSetting());
}

Id MesonToolKitAspect::mesonToolId(const Kit *kit)
{
    QTC_ASSERT(kit, return {});
    return Id::fromSetting(kit->value(MESON_TOOL_ID));
}

std::shared_ptr<ToolWrapper> MesonToolKitAspect::mesonTool(const Kit *kit)
{
    return MesonTools::mesonWrapper(MesonToolKitAspect::mesonToolId(kit));
}

bool MesonToolKitAspect::isValid(const Kit *kit)
{
    auto tool = mesonTool(kit);
    return tool && tool->isValid();
}

// MesonToolKitAspectFactory

class MesonToolKitAspectFactory final : public KitAspectFactory
{
public:
    MesonToolKitAspectFactory()
    {
        setId(MESON_TOOL_ID);
        setDisplayName(Tr::tr("Meson Tool"));
        setDescription(Tr::tr("The Meson tool to use when building a project with Meson.<br>"
                              "This setting is ignored when using other build systems."));
        setPriority(9000);
    }

    Tasks validate(const Kit *k) const final
    {
        Tasks tasks;
        const auto tool = MesonToolKitAspect::mesonTool(k);
        if (tool && !tool->isValid())
            tasks << BuildSystemTask{Task::Warning, Tr::tr("Cannot validate this meson executable.")};
        return tasks;
    }

    void setup(Kit *k) final
    {
        const auto tool = MesonToolKitAspect::mesonTool(k);
        if (!tool) {
            const auto autoDetected = MesonTools::autoDetectedMeson();
            if (autoDetected)
                MesonToolKitAspect::setMesonTool(k, autoDetected->id());
        }
    }
    void fix(Kit *k) final
    {
        setup(k);
    }

    KitAspect *createKitAspect(Kit *k) const final
    {
        return new MesonToolKitAspectImpl(k, this, ToolType::Meson);
    }

    ItemList toUserOutput(const Kit *k) const final
    {
        const auto tool = MesonToolKitAspect::mesonTool(k);
        if (tool)
            return {{Tr::tr("Meson"), tool->name()}};
        return {{Tr::tr("Meson"), Tr::tr("Unconfigured")}};
    }
};

const MesonToolKitAspectFactory theMesonKitAspectFactory;


// NinjaToolKitAspect

const char NINJA_TOOL_ID[] = "MesonProjectManager.MesonKitInformation.Ninja";

void NinjaToolKitAspect::setNinjaTool(Kit *kit, Id id)
{
    QTC_ASSERT(kit && id.isValid(), return );
    kit->setValue(NINJA_TOOL_ID, id.toSetting());
}

Id NinjaToolKitAspect::ninjaToolId(const Kit *kit)
{
    QTC_ASSERT(kit, return {});
    return Id::fromSetting(kit->value(NINJA_TOOL_ID));
}

std::shared_ptr<ToolWrapper> NinjaToolKitAspect::ninjaTool(const Kit *kit)
{
    return MesonTools::ninjaWrapper(NinjaToolKitAspect::ninjaToolId(kit));
}

bool NinjaToolKitAspect::isValid(const Kit *kit)
{
    auto tool = ninjaTool(kit);
    return tool && tool->isValid();
}

// NinjaToolKitAspectFactory

class NinjaToolKitAspectFactory final : public KitAspectFactory
{
public:
    NinjaToolKitAspectFactory()
    {
        setId(NINJA_TOOL_ID);
        setDisplayName(Tr::tr("Ninja Tool"));
        setDescription(Tr::tr("The Ninja tool to use when building a project with Meson.<br>"
                              "This setting is ignored when using other build systems."));
        setPriority(9000);
    }

    Tasks validate(const Kit *k) const final
    {
        Tasks tasks;
        const auto tool = NinjaToolKitAspect::ninjaTool(k);
        if (tool && !tool->isValid())
            tasks << BuildSystemTask{Task::Warning, Tr::tr("Cannot validate this Ninja executable.")};
        return tasks;
    }

    void setup(Kit *k) final
    {
        const auto tool = NinjaToolKitAspect::ninjaTool(k);
        if (!tool) {
            const auto autoDetected = MesonTools::autoDetectedNinja();
            if (autoDetected)
                NinjaToolKitAspect::setNinjaTool(k, autoDetected->id());
        }
    }
    void fix(Kit *k) final
    {
        setup(k);
    }

    ItemList toUserOutput(const Kit *k) const final
    {
        const auto tool = NinjaToolKitAspect::ninjaTool(k);
        if (tool)
            return {{Tr::tr("Ninja"), tool->name()}};
        return {{Tr::tr("Ninja"), Tr::tr("Unconfigured")}};
    }

    KitAspect *createKitAspect(Kit *k) const final
    {
        return new MesonToolKitAspectImpl(k, this, ToolType::Ninja);
    }
};

const NinjaToolKitAspectFactory theNinjaToolKitAspectFactory;

} // MesonProjectManager::Internal
