// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolssettingspage.h"

#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"
#include "toolitemsettings.h"
#include "toolsmodel.h"
#include "tooltreeitem.h"

#include <utils/detailswidget.h>
#include <utils/layoutbuilder.h>

#include <QHeaderView>
#include <QPushButton>
#include <QTreeView>

using namespace Utils;

namespace MesonProjectManager::Internal {

class ToolsSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    ToolsSettingsWidget();

private:
    void apply() final { m_model.apply(); }

    void cloneMesonTool();
    void removeMesonTool();
    void currentMesonToolChanged(const QModelIndex &newCurrent);

    ToolsModel m_model;
    ToolItemSettings *m_itemSettings;
    ToolTreeItem *m_currentItem = nullptr;

    QTreeView *m_mesonList;
    DetailsWidget *m_mesonDetails;
    QPushButton *m_cloneButton;
    QPushButton *m_removeButton;
};

ToolsSettingsWidget::ToolsSettingsWidget()
{
    m_mesonList = new QTreeView;
    m_mesonList->setModel(&m_model);
    m_mesonList->expandAll();
    m_mesonList->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_mesonList->header()->setSectionResizeMode(1, QHeaderView::Stretch);

    m_itemSettings = new ToolItemSettings;

    m_mesonDetails = new DetailsWidget;
    m_mesonDetails->setState(DetailsWidget::NoSummary);
    m_mesonDetails->setVisible(false);
    m_mesonDetails->setWidget(m_itemSettings);

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

    using namespace Layouting;

    Row {
        Column {
            m_mesonList,
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

    connect(m_mesonList->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ToolsSettingsWidget::currentMesonToolChanged);
    connect(m_itemSettings, &ToolItemSettings::applyChanges, &m_model, &ToolsModel::updateItem);

    connect(addButton, &QPushButton::clicked, &m_model, &ToolsModel::addMesonTool);
    connect(m_cloneButton, &QPushButton::clicked, this, &ToolsSettingsWidget::cloneMesonTool);
    connect(m_removeButton, &QPushButton::clicked, this, &ToolsSettingsWidget::removeMesonTool);
}

void ToolsSettingsWidget::cloneMesonTool()
{
    if (m_currentItem) {
        auto newItem = m_model.cloneMesonTool(m_currentItem);
        m_mesonList->setCurrentIndex(newItem->index());
    }
}

void ToolsSettingsWidget::removeMesonTool()
{
    if (m_currentItem)
        m_model.removeMesonTool(m_currentItem);
}

void ToolsSettingsWidget::currentMesonToolChanged(const QModelIndex &newCurrent)
{
    m_currentItem = m_model.mesoneToolTreeItem(newCurrent);
    m_itemSettings->load(m_currentItem);
    m_mesonDetails->setVisible(m_currentItem);
    m_cloneButton->setEnabled(m_currentItem);
    m_removeButton->setEnabled(m_currentItem && !m_currentItem->isAutoDetected());
}


ToolsSettingsPage::ToolsSettingsPage()
{
    setId(Constants::SettingsPage::TOOLS_ID);
    setDisplayName(Tr::tr("Tools"));
    setCategory(Constants::SettingsPage::CATEGORY);
    setWidgetCreator([]() { return new ToolsSettingsWidget; });
}

} // namespace MesonProjectManager
