// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolkitaspectwidget.h"

#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"
#include "mesontools.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspect.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QCoreApplication>

using namespace ProjectExplorer;
using namespace Utils;

namespace MesonProjectManager::Internal {

// Meson KitAspect base

class MesonToolKitAspectImpl final : public KitAspect
{
public:
    MesonToolKitAspectImpl(Kit *kit, const KitAspectFactory *factory);
    ~MesonToolKitAspectImpl() { delete m_toolsComboBox; }

private:
    void addTool(const MesonTools::Tool_t &tool);
    void removeTool(const MesonTools::Tool_t &tool);
    void setCurrentToolIndex(int index);
    int indexOf(const Id &id);
    void loadTools();
    void setToDefault();

    void makeReadOnly() final { m_toolsComboBox->setEnabled(false); }

    void addToInnerLayout(Layouting::Layout &layout) final
    {
        addMutableAction(m_toolsComboBox);
        layout.addItem(m_toolsComboBox);
    }

    void refresh() final
    {
        const auto id = MesonToolKitAspect::mesonToolId(kit());
        m_toolsComboBox->setCurrentIndex(indexOf(id));
    }

    QComboBox *m_toolsComboBox;
};

MesonToolKitAspectImpl::MesonToolKitAspectImpl(Kit *kit, const KitAspectFactory *factory)
    : KitAspect(kit, factory)
    , m_toolsComboBox(createSubWidget<QComboBox>())
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
        m_toolsComboBox->addItem(tool->name(), tool->id().toSetting());
}

void MesonToolKitAspectImpl::removeTool(const MesonTools::Tool_t &tool)
{
    QTC_ASSERT(tool, return );
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
    MesonToolKitAspect::setMesonTool(kit(), id);
}

int MesonToolKitAspectImpl::indexOf(const Id &id)
{
    for (int i = 0; i < m_toolsComboBox->count(); ++i) {
        if (id == Id::fromSetting(m_toolsComboBox->itemData(i)))
            return i;
    }
    return -1;
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
    const MesonTools::Tool_t autoDetected = MesonTools::autoDetectedTool();

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

std::shared_ptr<MesonToolWrapper> MesonToolKitAspect::mesonTool(const Kit *kit)
{
    return MesonTools::toolById(MesonToolKitAspect::mesonToolId(kit));
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
            const auto autoDetected = MesonTools::autoDetectedTool();
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
        return new MesonToolKitAspectImpl(k, this);
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


} // MesonProjectManager::Internal
