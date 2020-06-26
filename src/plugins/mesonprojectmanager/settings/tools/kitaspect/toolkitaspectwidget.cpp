/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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

#include "toolkitaspectwidget.h"
#include "coreplugin/icore.h"
#include "mesonpluginconstants.h"
#include "mesontoolkitaspect.h"
#include "ninjatoolkitaspect.h"
#include "utils/qtcassert.h"

namespace MesonProjectManager {
namespace Internal {
ToolKitAspectWidget::ToolKitAspectWidget(ProjectExplorer::Kit *kit,
                                         const ProjectExplorer::KitAspect *ki,
                                         ToolType type)
    : ProjectExplorer::KitAspectWidget(kit, ki)
    , m_toolsComboBox{new QComboBox}
    , m_manageButton(new QPushButton(KitAspectWidget::msgManage()))
    , m_type{type}
{
    m_toolsComboBox->setSizePolicy(QSizePolicy::Ignored,
                                   m_toolsComboBox->sizePolicy().verticalPolicy());
    m_toolsComboBox->setEnabled(false);
    m_toolsComboBox->setToolTip(ki->description());
    loadTools();

    m_manageButton->setContentsMargins(0, 0, 0, 0);
    connect(m_manageButton, &QPushButton::clicked, this, [this]() {
        Core::ICore::showOptionsDialog(Constants::SettingsPage::TOOLS_ID, buttonWidget());
    });

    connect(MesonTools::instance(),
            &MesonTools::toolAdded,
            this,
            &ToolKitAspectWidget::addTool);
    connect(MesonTools::instance(),
            &MesonTools::toolRemoved,
            this,
            &ToolKitAspectWidget::removeTool);
    connect(m_toolsComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &ToolKitAspectWidget::setCurrentToolIndex);
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
    std::for_each(std::cbegin(MesonTools::tools()),
                  std::cend(MesonTools::tools()),
                  [this](const MesonTools::Tool_t &tool) { addTool(tool); });
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
