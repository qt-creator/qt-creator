// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolssettingspage.h"

#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"
#include "mesontools.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/aspects.h>
#include <utils/detailswidget.h>
#include <utils/groupedmodel.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QPushButton>

using namespace Utils;

namespace MesonProjectManager::Internal {

class ToolItem
{
public:
    ToolItem() = default;
    explicit ToolItem(const QString &name);
    explicit ToolItem(const MesonTools::Tool_t &tool);
    ToolItem cloned() const;

    QVariant data(int column, int role) const;

    friend bool operator==(const ToolItem &, const ToolItem &) = default;

    QString name;
    FilePath executable;
    Id id;
    bool autoDetected = false;
};

} // namespace MesonProjectManager::Internal

Q_DECLARE_METATYPE(MesonProjectManager::Internal::ToolItem)

namespace MesonProjectManager::Internal {

ToolItem::ToolItem(const QString &name)
    : name{name}
    , id{Id::generate()}
    , autoDetected{false}
{}

ToolItem::ToolItem(const MesonTools::Tool_t &tool)
    : name{tool->name()}
    , executable{tool->exe()}
    , id{tool->id()}
    , autoDetected{tool->autoDetected()}
{}

ToolItem ToolItem::cloned() const
{
    ToolItem result;
    result.name = Tr::tr("Clone of %1").arg(name);
    result.executable = executable;
    result.id = Id::generate();
    result.autoDetected = false;
    return result;
}

QVariant ToolItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (column) {
        case 0:
            return name;
        case 1:
            return executable.toUserOutput();
        }
        return {};
    case Qt::ToolTipRole: {
        if (!executable.exists())
            return Tr::tr("Meson executable path does not exist.");
        if (!executable.isFile())
            return Tr::tr("Meson executable path is not a file.");
        if (!executable.isExecutableFile())
            return Tr::tr("Meson executable path is not executable.");
        const QVersionNumber ver = MesonToolWrapper::read_version(executable);
        return ver.isNull() ? Tr::tr("Cannot get tool version.")
                            : Tr::tr("Version: %1").arg(ver.toString());
    }
    case Qt::DecorationRole:
        if (column == 0 && !executable.isExecutableFile())
            return Icons::CRITICAL.icon();
        return {};
    }
    return {};
}

// ToolsModel

class ToolsModel final : public TypedGroupedModel<ToolItem>
{
public:
    ToolsModel();

    int addMesonTool();
    int cloneMesonTool(int row);
    void updateItem(const Id &itemId, const QString &name, const FilePath &exe);
    void apply() override;

private:
    QVariant variantData(const QVariant &v, int column, int role) const override;
    QString uniqueName(const QString &baseName) const;
    int rowForId(const Id &id) const;
};

ToolsModel::ToolsModel()
{
    setHeader({Tr::tr("Name"), Tr::tr("Location")});
    setFilters(ProjectExplorer::Constants::msgAutoDetected(),
               {{ProjectExplorer::Constants::msgManual(), [this](int row) {
                    return !item(row).autoDetected;
                }}});
    for (const MesonTools::Tool_t &tool : MesonTools::tools())
        appendItem(ToolItem{tool});
}

int ToolsModel::addMesonTool()
{
    return appendVolatileItem(ToolItem{uniqueName(Tr::tr("New Meson"))});
}

int ToolsModel::cloneMesonTool(int row)
{
    return appendVolatileItem(item(row).cloned());
}

void ToolsModel::updateItem(const Id &itemId, const QString &name, const FilePath &exe)
{
    const int row = rowForId(itemId);
    QTC_ASSERT(row >= 0, return);
    ToolItem it = item(row);
    it.name = name;
    it.executable = exe;
    setVolatileItem(row, it);
    notifyRowChanged(row);
}

int ToolsModel::rowForId(const Id &id) const
{
    for (int row = 0; row < itemCount(); ++row) {
        if (item(row).id == id)
            return row;
    }
    return -1;
}

void ToolsModel::apply()
{
    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row)) {
            MesonTools::removeTool(item(row).id);
            continue;
        }
        if (isDirty(row)) {
            const ToolItem it = item(row);
            MesonTools::updateTool(it.id, it.name, it.executable);
        }
    }

    GroupedModel::apply();
}

QVariant ToolsModel::variantData(const QVariant &v, int column, int role) const
{
    return fromVariant(v).data(column, role);
}

QString ToolsModel::uniqueName(const QString &baseName) const
{
    QStringList names;
    for (int row = 0; row < itemCount(); ++row)
        names << item(row).name;
    return Utils::makeUniquelyNumbered(baseName, names);
}

// ToolsSettingsWidget

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
    bool m_loading = false;

    QPushButton m_addButton;
    QPushButton m_cloneButton;
    QPushButton m_removeButton;
    QPushButton m_makeDefaultButton;

    DetailsWidget m_mesonDetails;
    QWidget m_itemConfigWidget;
    AspectContainer m_data;
    StringAspect m_name{&m_data};
    FilePathAspect m_executable{&m_data};
};

ToolsSettingsWidget::ToolsSettingsWidget()
{
    m_name.setDisplayStyle(StringAspect::LineEditDisplay);
    m_name.setLabelText(Tr::tr("Name:"));

    m_executable.setExpectedKind(PathChooser::ExistingCommand);
    m_executable.setHistoryCompleter("Meson.Command.History");
    m_executable.setLabelText(Tr::tr("Path:"));

    using namespace Layouting;
    Form {
        m_name, br,
        m_executable, br, noMargin
    }.attachTo(&m_itemConfigWidget);

    m_mesonDetails.setState(DetailsWidget::NoSummary);
    m_mesonDetails.setVisible(false);
    m_mesonDetails.setWidget(&m_itemConfigWidget);

    m_addButton.setText(Tr::tr("Add"));
    m_cloneButton.setText(Tr::tr("Clone"));
    m_cloneButton.setEnabled(false);
    m_removeButton.setText(Tr::tr("Remove"));
    m_removeButton.setEnabled(false);
    m_makeDefaultButton.setText(Tr::tr("Make Default"));
    m_makeDefaultButton.setEnabled(false);
    m_makeDefaultButton.setVisible(false);
    m_makeDefaultButton.setToolTip(Tr::tr("Set as the default Meson executable to use "
                                          "when creating a new kit or when no value is set."));

    Row {
        Column { m_groupedView.view(), m_mesonDetails },
        Column { m_addButton, m_cloneButton, m_removeButton, m_makeDefaultButton, st }
    }.attachTo(this);

    connect(&m_groupedView, &GroupedView::currentRowChanged,
            this, &ToolsSettingsWidget::currentMesonToolChanged);

    m_name.addOnVolatileValueChanged(this, [this] { store(); });
    m_executable.addOnVolatileValueChanged(this, [this] { store(); });

    connect(&m_addButton, &QPushButton::clicked, this, [this] {
        m_groupedView.selectRow(m_model.addMesonTool());
        markSettingsDirty();
    });
    connect(&m_cloneButton, &QPushButton::clicked, this, &ToolsSettingsWidget::cloneMesonTool);
    connect(&m_removeButton, &QPushButton::clicked, this, &ToolsSettingsWidget::removeMesonTool);

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
                           m_name.volatileValue(),
                           m_executable.expandedVolatileValue());
    }
}

void ToolsSettingsWidget::currentMesonToolChanged(int, int newRow)
{
    const bool hasRow = newRow >= 0;
    const bool hasItem = hasRow && !m_model.isRemoved(newRow);
    m_loading = true;
    if (hasItem) {
        const ToolItem &it = m_model.item(newRow);
        m_name.setEnabled(!it.autoDetected);
        m_name.setValue(it.name);
        m_executable.setEnabled(!it.autoDetected);
        m_executable.setValue(it.executable);
    }
    m_loading = false;
    m_mesonDetails.setVisible(hasItem);
    m_cloneButton.setEnabled(hasItem);
    m_removeButton.setEnabled(hasRow && !m_model.item(newRow).autoDetected);
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
