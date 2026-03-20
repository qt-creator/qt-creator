// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolssettingspage.h"

#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"
#include "toolsmodel.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/detailswidget.h>
#include <utils/layoutbuilder.h>

#include <QHeaderView>
#include <QPushButton>
#include <QTreeView>

#include <optional>

using namespace Utils;

namespace MesonProjectManager::Internal {

class ToolItemSettings final : public QWidget
{
    Q_OBJECT

public:
    ToolItemSettings()
    {
        m_mesonNameLineEdit = new QLineEdit;

        m_mesonPathChooser = new PathChooser;
        m_mesonPathChooser->setExpectedKind(PathChooser::ExistingCommand);
        m_mesonPathChooser->setHistoryCompleter("Meson.Command.History");

        using namespace Layouting;

        Form {
            Tr::tr("Name:"), m_mesonNameLineEdit, br,
            Tr::tr("Path:"), m_mesonPathChooser, br,
            noMargin
        }.attachTo(this);

        connect(m_mesonPathChooser, &PathChooser::rawPathChanged, this, &ToolItemSettings::store);
        connect(m_mesonNameLineEdit, &QLineEdit::textChanged, this, &ToolItemSettings::store);
    }

    void load(std::optional<ToolItem> item)
    {
        m_currentId = std::nullopt;
        if (item) {
            m_mesonNameLineEdit->setDisabled(item->autoDetected);
            m_mesonNameLineEdit->setText(item->name);
            m_mesonPathChooser->setDisabled(item->autoDetected);
            m_mesonPathChooser->setFilePath(item->executable);
            m_currentId = item->id;
        }
    }

    void store()
    {
        if (m_currentId) {
            emit applyChanges(*m_currentId,
                              m_mesonNameLineEdit->text(),
                              m_mesonPathChooser->filePath());
        }
    }

signals:
    void applyChanges(Id itemId, const QString &name, const FilePath &exe);

private:
    std::optional<Id> m_currentId{std::nullopt};
    QLineEdit *m_mesonNameLineEdit;
    PathChooser *m_mesonPathChooser;
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

    ToolsModel m_model;
    GroupedView m_groupedView{m_model};
    ToolItemSettings *m_itemSettings;

    DetailsWidget *m_mesonDetails;
    QPushButton *m_cloneButton;
    QPushButton *m_removeButton;
};

ToolsSettingsWidget::ToolsSettingsWidget()
{
    m_groupedView.view().header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_groupedView.view().header()->setSectionResizeMode(1, QHeaderView::Stretch);

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
    connect(m_itemSettings, &ToolItemSettings::applyChanges, &m_model, &ToolsModel::updateItem);

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

void ToolsSettingsWidget::currentMesonToolChanged(int, int newRow)
{
    const bool hasRow = newRow >= 0;
    const bool hasItem = hasRow && !m_model.isRemoved(newRow);
    m_itemSettings->load(hasItem ? std::optional<ToolItem>{m_model.item(newRow)}
                                 : std::nullopt);
    m_mesonDetails->setVisible(hasItem);
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

#include "toolssettingspage.moc"
