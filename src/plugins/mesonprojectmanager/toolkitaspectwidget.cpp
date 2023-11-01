// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolkitaspectwidget.h"

#include "mesonpluginconstants.h"
#include "mesontoolkitaspect.h"
#include "ninjatoolkitaspect.h"

#include <utils/qtcassert.h>

namespace MesonProjectManager {
namespace Internal {

ToolKitAspectWidget::ToolKitAspectWidget(ProjectExplorer::Kit *kit,
                                         const ProjectExplorer::KitAspectFactory *factory,
                                         ToolType type)
    : ProjectExplorer::KitAspect(kit, factory)
    , m_toolsComboBox(createSubWidget<QComboBox>())
    , m_manageButton(createManageButton(Constants::SettingsPage::TOOLS_ID))
    , m_type{type}
{
    m_toolsComboBox->setSizePolicy(QSizePolicy::Ignored,
                                   m_toolsComboBox->sizePolicy().verticalPolicy());
    m_toolsComboBox->setEnabled(false);
    m_toolsComboBox->setToolTip(factory->description());
    loadTools();

    connect(MesonTools::instance(), &MesonTools::toolAdded,
            this, &ToolKitAspectWidget::addTool);
    connect(MesonTools::instance(), &MesonTools::toolRemoved,
            this, &ToolKitAspectWidget::removeTool);
    connect(m_toolsComboBox, &QComboBox::currentIndexChanged,
            this, &ToolKitAspectWidget::setCurrentToolIndex);
}

ToolKitAspectWidget::~ToolKitAspectWidget()
{
    delete m_toolsComboBox;
    delete m_manageButton;
}

void ToolKitAspectWidget::addTool(const MesonTools::Tool_t &tool)
{
    QTC_ASSERT(tool, return );
    if (isCompatible(tool))
        this->m_toolsComboBox->addItem(tool->name(), tool->id().toSetting());
}

void ToolKitAspectWidget::removeTool(const MesonTools::Tool_t &tool)
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

void ToolKitAspectWidget::setCurrentToolIndex(int index)
{
    if (m_toolsComboBox->count() == 0)
        return;
    const Utils::Id id = Utils::Id::fromSetting(m_toolsComboBox->itemData(index));
    if (m_type == ToolType::Meson)
        MesonToolKitAspect::setMesonTool(m_kit, id);
    else
        NinjaToolKitAspect::setNinjaTool(m_kit, id);
}

int ToolKitAspectWidget::indexOf(const Utils::Id &id)
{
    for (int i = 0; i < m_toolsComboBox->count(); ++i) {
        if (id == Utils::Id::fromSetting(m_toolsComboBox->itemData(i)))
            return i;
    }
    return -1;
}

bool ToolKitAspectWidget::isCompatible(const MesonTools::Tool_t &tool)
{
    return (m_type == ToolType::Meson && MesonTools::isMesonWrapper(tool))
           || (m_type == ToolType::Ninja && MesonTools::isNinjaWrapper(tool));
}

void ToolKitAspectWidget::loadTools()
{
    for (const MesonTools::Tool_t &tool : MesonTools::tools()) {
        addTool(tool);
    }
    refresh();
    m_toolsComboBox->setEnabled(m_toolsComboBox->count());
}

void ToolKitAspectWidget::setToDefault()
{
    const MesonTools::Tool_t autoDetected = [this]() {
        if (m_type == ToolType::Meson)
            return std::dynamic_pointer_cast<ToolWrapper>(MesonTools::mesonWrapper());
        return std::dynamic_pointer_cast<ToolWrapper>(MesonTools::ninjaWrapper());
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

} // namespace Internal
} // namespace MesonProjectManager
