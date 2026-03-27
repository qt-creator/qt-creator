// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolssettingspage.h"

#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"
#include "toolsmodel.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>
#include <utils/detailswidget.h>
#include <utils/layoutbuilder.h>

#include <QHeaderView>
#include <QPushButton>

using namespace Utils;

namespace MesonProjectManager::Internal {

class ToolItemData final : public AspectContainer
{
public:
    ToolItemData()
    {
        name.setDisplayStyle(StringAspect::LineEditDisplay);
        name.setLabelText(Tr::tr("Name:"));

        executable.setExpectedKind(PathChooser::ExistingCommand);
        executable.setHistoryCompleter("Meson.Command.History");
        executable.setLabelText(Tr::tr("Path:"));
    }

    StringAspect name{this};
    FilePathAspect executable{this};
};

class ToolsSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    ToolsSettingsWidget();

private:
    void apply() final { m_model.apply(); }

    void cloneMesonTool();
    void removeMesonTool();
    void currentMesonToolChanged(int oldRow, int newRow);
    void store();

    ToolsModel m_model;
    GroupedView m_groupedView{m_model};
    ToolItemData m_data;
    bool m_loading = false;

    DetailsWidget m_mesonDetails;
    QPushButton *m_cloneButton = nullptr;
    QPushButton *m_removeButton = nullptr;
};

ToolsSettingsWidget::ToolsSettingsWidget()
{
    m_groupedView.view().header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_groupedView.view().header()->setSectionResizeMode(1, QHeaderView::Stretch);

    auto inner = new QWidget;
    using namespace Layouting;
    Form {
        m_data.name, br,
        m_data.executable, br, noMargin
    }.attachTo(inner);

    m_mesonDetails.setState(DetailsWidget::NoSummary);
    m_mesonDetails.setVisible(false);
    m_mesonDetails.setWidget(inner);

    auto addButton = new QPushButton(Tr::tr("Add"));

    m_cloneButton = new QPushButton(Tr::tr("Clone"));
    m_cloneButton->setEnabled(false);

    m_removeButton = new QPushButton(Tr::tr("Remove"));
    m_removeButton->setEnabled(false);

    auto makeDefaultButton = new QPushButton(Tr::tr("Make Default"));
    makeDefaultButton->setEnabled(false);
    makeDefaultButton->setVisible(false);
    makeDefaultButton->setToolTip(Tr::tr("Set as the default Meson executable to use "
                                         "when creating a new kit or when no value is set."));

    Row {
        Column {
            m_groupedView.view(),
            m_mesonDetails
        },
        Column {
            addButton,
            m_cloneButton,
            m_removeButton,
            makeDefaultButton,
            st
        }
    }.attachTo(this);

    connect(&m_groupedView, &GroupedView::currentRowChanged,
            this, &ToolsSettingsWidget::currentMesonToolChanged);

    m_data.name.addOnVolatileValueChanged(this, [this] { store(); });
    m_data.executable.addOnVolatileValueChanged(this, [this] { store(); });

    connect(addButton, &QPushButton::clicked, this, [this] {
        m_groupedView.selectRow(m_model.addMesonTool());
        markSettingsDirty();
    });
    connect(m_cloneButton, &QPushButton::clicked, this, &ToolsSettingsWidget::cloneMesonTool);
    connect(m_removeButton, &QPushButton::clicked, this, &ToolsSettingsWidget::removeMesonTool);

    installMarkSettingsDirtyTriggerRecursively(this);
}

void ToolsSettingsWidget::cloneMesonTool()
{
    const int row = m_groupedView.currentRow();
    if (row >= 0) {
        m_groupedView.selectRow(m_model.cloneMesonTool(row));
        markSettingsDirty();
    }
}

void ToolsSettingsWidget::removeMesonTool()
{
    const int row = m_groupedView.currentRow();
    if (row >= 0) {
        m_model.markRemoved(row);
        markSettingsDirty();
    }
}

void ToolsSettingsWidget::store()
{
    if (m_loading)
        return;
    const int row = m_groupedView.currentRow();
    if (row >= 0 && !m_model.isRemoved(row)) {
        m_model.updateItem(m_model.item(row).id,
                           m_data.name.volatileValue(),
                           FilePath::fromUserInput(m_data.executable.volatileValue()));
    }
}

void ToolsSettingsWidget::currentMesonToolChanged(int, int newRow)
{
    const bool hasRow = newRow >= 0;
    const bool hasItem = hasRow && !m_model.isRemoved(newRow);
    m_loading = true;
    if (hasItem) {
        const ToolItem &it = m_model.item(newRow);
        m_data.name.setEnabled(!it.autoDetected);
        m_data.name.setValue(it.name);
        m_data.executable.setEnabled(!it.autoDetected);
        m_data.executable.setValue(it.executable);
    }
    m_loading = false;
    m_mesonDetails.setVisible(hasItem);
    m_cloneButton->setEnabled(hasItem);
    m_removeButton->setEnabled(hasRow && !m_model.item(newRow).autoDetected);
}

class ToolsSettingsPage final : public Core::IOptionsPage
{
public:
    ToolsSettingsPage()
    {
        setId(Constants::SettingsPage::TOOLS_ID);
        setDisplayName(Tr::tr("Tools"));
        setCategory(Constants::SettingsPage::CATEGORY);
        setWidgetCreator([]() { return new ToolsSettingsWidget; });
    }
};

void setupToolsSettingsPage()
{
    static ToolsSettingsPage theToolsSettingsPage;
}

} // namespace MesonProjectManager
