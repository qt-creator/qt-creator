// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gntoolssettingspage.h"

#include "gnpluginconstants.h"
#include "gnprojectmanagertr.h"
#include "gntools.h"

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
using namespace ProjectExplorer;

namespace GNProjectManager::Internal {

class GNToolItem final
{
public:
    GNToolItem() = default;
    explicit GNToolItem(const QString &name);
    explicit GNToolItem(const GNTools::Tool_t &tool);
    GNToolItem cloned() const;

    QVariant data(int column, int role) const;
    friend bool operator==(const GNToolItem &, const GNToolItem &) = default;

    QString name;
    FilePath executable;
    Id id;
    bool autoDetected = false;
};

} // namespace GNProjectManager::Internal

Q_DECLARE_METATYPE(GNProjectManager::Internal::GNToolItem)

namespace GNProjectManager::Internal {

GNToolItem::GNToolItem(const QString &name)
    : name{name}
    , id{Id::generate()}
    , autoDetected{false}
{}

GNToolItem::GNToolItem(const GNTools::Tool_t &tool)
    : name{tool->name()}
    , executable{tool->exe()}
    , id{tool->id()}
    , autoDetected{tool->autoDetected()}
{}

GNToolItem GNToolItem::cloned() const
{
    GNToolItem result;
    result.name = Tr::tr("Clone of %1").arg(name);
    result.executable = executable;
    result.id = Id::generate();
    result.autoDetected = false;
    return result;
}

QVariant GNToolItem::data(int column, int role) const
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
            return Tr::tr("GN executable path does not exist.");
        if (!executable.isFile())
            return Tr::tr("GN executable path is not a file.");
        if (!executable.isExecutableFile())
            return Tr::tr("GN executable path is not executable.");
        const QVersionNumber ver = GNTool::readVersion(executable);
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


// GNToolsModel

class GNToolsModel final : public TypedGroupedModel<GNToolItem>
{
public:
    GNToolsModel();

    int addGNTool();
    int cloneGNTool(int row);
    void updateItem(const Id &itemId, const QString &name, const FilePath &exe);
    int rowForId(const Id &id) const;
    void apply() override;

private:
    QVariant variantData(const QVariant &v, int column, int role) const override;
    QString uniqueName(const QString &baseName) const;
};

GNToolsModel::GNToolsModel()
{
    setHeader({Tr::tr("Name"), Tr::tr("Location")});
    setFilters(ProjectExplorer::Constants::msgAutoDetected(),
               {{ProjectExplorer::Constants::msgManual(), [this](int row) {
                    return !item(row).autoDetected;
                }}});
    for (const GNTools::Tool_t &tool : GNTools::tools())
        appendItem(GNToolItem{tool});
}

int GNToolsModel::addGNTool()
{
    return appendVolatileItem(GNToolItem{uniqueName(Tr::tr("New GN"))});
}

int GNToolsModel::cloneGNTool(int row)
{
    return appendVolatileItem(item(row).cloned());
}

void GNToolsModel::updateItem(const Id &itemId, const QString &name, const FilePath &exe)
{
    const int row = rowForId(itemId);
    QTC_ASSERT(row >= 0, return);
    GNToolItem it = item(row);
    it.name = name;
    it.executable = exe;
    setVolatileItem(row, it);
    notifyRowChanged(row);
}

int GNToolsModel::rowForId(const Id &id) const
{
    for (int row = 0; row < itemCount(); ++row) {
        if (item(row).id == id)
            return row;
    }
    return -1;
}

void GNToolsModel::apply()
{
    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row)) {
            GNTools::removeTool(item(row).id);
            continue;
        }
        if (isDirty(row)) {
            const GNToolItem it = item(row);
            GNTools::updateTool(it.id, it.name, it.executable);
        }
    }

    GroupedModel::apply();
}

QVariant GNToolsModel::variantData(const QVariant &v, int column, int role) const
{
    return fromVariant(v).data(column, role);
}

QString GNToolsModel::uniqueName(const QString &baseName) const
{
    QStringList names;
    for (int row = 0; row < itemCount(); ++row)
        names << item(row).name;
    return Utils::makeUniquelyNumbered(baseName, names);
}

// GNToolsSettingsWidget

class GNToolItemData final : public AspectContainer
{
public:
    GNToolItemData()
    {
        name.setDisplayStyle(StringAspect::LineEditDisplay);
        name.setLabelText(Tr::tr("Name:"));

        executable.setExpectedKind(PathChooser::ExistingCommand);
        executable.setHistoryCompleter("GN.Command.History");
        executable.setLabelText(Tr::tr("Path:"));
    }

    StringAspect name{this};
    FilePathAspect executable{this};
};

class GNToolsSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    GNToolsSettingsWidget();

private:
    void apply() final { m_model.apply(); }

    void cloneGNTool();
    void removeGNTool();
    void currentToolChanged(int oldRow, int newRow);
    void store();

    GNToolsModel m_model;
    GroupedView m_groupedView{m_model};
    GNToolItemData m_data;
    bool m_loading = false;

    DetailsWidget m_gnDetails;
    QPushButton m_addButton;
    QPushButton m_cloneButton;
    QPushButton m_removeButton;
};

GNToolsSettingsWidget::GNToolsSettingsWidget()
{
    auto inner = new QWidget;
    using namespace Layouting;
    Form {
        m_data.name, br,
        m_data.executable, br, noMargin
    }.attachTo(inner);

    m_gnDetails.setState(DetailsWidget::NoSummary);
    m_gnDetails.setVisible(false);
    m_gnDetails.setWidget(inner);

    m_addButton.setText(Tr::tr("Add"));
    m_cloneButton.setText(Tr::tr("Clone"));
    m_cloneButton.setEnabled(false);
    m_removeButton.setText(Tr::tr("Remove"));
    m_removeButton.setEnabled(false);

    Row {
        Column { m_groupedView.view(), m_gnDetails },
        Column { m_addButton, m_cloneButton, m_removeButton, st }
    }.attachTo(this);

    connect(&m_groupedView, &GroupedView::currentRowChanged,
            this, &GNToolsSettingsWidget::currentToolChanged);

    m_data.name.addOnVolatileValueChanged(this, [this] { store(); });
    m_data.executable.addOnVolatileValueChanged(this, [this] { store(); });

    connect(&m_addButton, &QPushButton::clicked, this, [this] {
        m_groupedView.selectRow(m_model.addGNTool());
        markSettingsDirty();
    });
    connect(&m_cloneButton, &QPushButton::clicked, this, &GNToolsSettingsWidget::cloneGNTool);
    connect(&m_removeButton, &QPushButton::clicked, this, &GNToolsSettingsWidget::removeGNTool);

    installMarkSettingsDirtyTriggerRecursively(this);
}

void GNToolsSettingsWidget::cloneGNTool()
{
    const int row = m_groupedView.currentRow();
    if (row >= 0) {
        m_groupedView.selectRow(m_model.cloneGNTool(row));
        markSettingsDirty();
    }
}

void GNToolsSettingsWidget::removeGNTool()
{
    const int row = m_groupedView.currentRow();
    if (row >= 0) {
        m_model.markRemoved(row);
        markSettingsDirty();
    }
}

void GNToolsSettingsWidget::store()
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

void GNToolsSettingsWidget::currentToolChanged(int, int newRow)
{
    const bool hasRow = newRow >= 0;
    const bool hasItem = hasRow && !m_model.isRemoved(newRow);
    m_loading = true;
    if (hasItem) {
        const GNToolItem &it = m_model.item(newRow);
        m_data.name.setEnabled(!it.autoDetected);
        m_data.name.setValue(it.name);
        m_data.executable.setEnabled(!it.autoDetected);
        m_data.executable.setValue(it.executable);
    }
    m_loading = false;
    m_gnDetails.setVisible(hasItem);
    m_cloneButton.setEnabled(hasItem);
    m_removeButton.setEnabled(hasRow && !m_model.item(newRow).autoDetected);
}

// Setup

class GNToolsSettingsPage final : public Core::IOptionsPage
{
public:
    GNToolsSettingsPage()
    {
        setId(Constants::SettingsPage::TOOLS_ID);
        setDisplayName(Tr::tr("Tools"));
        setCategory(Constants::SettingsPage::CATEGORY);
        setWidgetCreator([] { return new GNToolsSettingsWidget; });
    }
};

void setupGNToolsSettingsPage()
{
    static GNToolsSettingsPage theGNToolsSettingsPage;
}

} // namespace GNProjectManager::Internal
