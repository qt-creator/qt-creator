// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "mesontoolkitaspect.h"
#include "ninjatoolkitaspect.h"

#include <projectexplorer/kitmanager.h>

#include <utils/layoutbuilder.h>

#include <QComboBox>
#include <QCoreApplication>

namespace MesonProjectManager {
namespace Internal {

class ToolKitAspectWidget final : public ProjectExplorer::KitAspect
{
public:
    enum class ToolType { Meson, Ninja };

    ToolKitAspectWidget(ProjectExplorer::Kit *kit,
                        const ProjectExplorer::KitAspectFactory *factory,
                        ToolType type);
    ~ToolKitAspectWidget();

private:
    void addTool(const MesonTools::Tool_t &tool);
    void removeTool(const MesonTools::Tool_t &tool);
    void setCurrentToolIndex(int index);
    int indexOf(const Utils::Id &id);
    bool isCompatible(const MesonTools::Tool_t &tool);
    void loadTools();
    void setToDefault();

    void makeReadOnly() override { m_toolsComboBox->setEnabled(false); }

    void addToLayoutImpl(Layouting::LayoutItem &parent) override
    {
        addMutableAction(m_toolsComboBox);
        parent.addItem(m_toolsComboBox);
        parent.addItem(m_manageButton);
    }

    void refresh() override
    {
        const auto id = [this]() {
            if (m_type == ToolType::Meson)
                return MesonToolKitAspect::mesonToolId(m_kit);
            return NinjaToolKitAspect::ninjaToolId(m_kit);
        }();
        if (id.isValid())
            m_toolsComboBox->setCurrentIndex(indexOf(id));
        else {
            setToDefault();
        }
    }

    QComboBox *m_toolsComboBox;
    QWidget *m_manageButton;
    ToolType m_type;
};

} // namespace Internal
} // namespace MesonProjectManager
