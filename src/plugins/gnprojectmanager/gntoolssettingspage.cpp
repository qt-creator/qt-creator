// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gntoolssettingspage.h"

#include "gnpluginconstants.h"
#include "gnprojectmanagertr.h"
#include "gntools.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/detailswidget.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QQueue>
#include <QTreeView>

using namespace Utils;
using namespace ProjectExplorer;

namespace GNProjectManager::Internal {
// ToolTreeItem

class GNToolTreeItem final : public TreeItem
{
public:
    GNToolTreeItem(const QString &name)
        : m_name{name}
        , m_id(Id::generate())
    {
        selfCheck();
        updateTooltip();
    }

    GNToolTreeItem(const GNTools::Tool_t &tool)
        : m_name{tool->name()}
        , m_executable{tool->exe()}
        , m_id{tool->id()}
        , m_autoDetected{tool->autoDetected()}
    {
        m_tooltip = Tr::tr("Version: %1").arg(tool->version().toString());
        selfCheck();
    }

    GNToolTreeItem(const GNToolTreeItem &other)
        : m_name{Tr::tr("Clone of %1").arg(other.m_name)}
        , m_executable{other.m_executable}
        , m_id{Id::generate()}
        , m_autoDetected{false}
        , m_unsavedChanges{true}
    {
        selfCheck();
        updateTooltip();
    }

    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DisplayRole:
            switch (column) {
            case 0:
                return m_name;
            case 1:
                return m_executable.toUserOutput();
            }
            return {};
        case Qt::FontRole: {
            QFont font;
            font.setBold(m_unsavedChanges);
            return font;
        }
        case Qt::ToolTipRole:
            if (!m_pathExists)
                return Tr::tr("GN executable path does not exist.");
            if (!m_pathIsFile)
                return Tr::tr("GN executable path is not a file.");
            if (!m_pathIsExecutable)
                return Tr::tr("GN executable path is not executable.");
            return m_tooltip;
        case Qt::DecorationRole:
            if (column == 0 && (!m_pathExists || !m_pathIsFile || !m_pathIsExecutable))
                return Icons::CRITICAL.icon();
            return {};
        }
        return {};
    }

    bool isAutoDetected() const noexcept { return m_autoDetected; }
    QString name() const noexcept { return m_name; }
    FilePath executable() const noexcept { return m_executable; }
    Id id() const noexcept { return m_id; }
    bool hasUnsavedChanges() const noexcept { return m_unsavedChanges; }
    void setSaved() { m_unsavedChanges = false; }

    void update(const QString &name, const FilePath &exe)
    {
        m_unsavedChanges = true;
        m_name = name;
        if (exe != m_executable) {
            m_executable = exe;
            selfCheck();
            updateTooltip();
        }
    }

private:
    void selfCheck()
    {
        m_pathExists = m_executable.exists();
        m_pathIsFile = m_executable.toFileInfo().isFile();
        m_pathIsExecutable = m_executable.toFileInfo().isExecutable();
    }

    void updateTooltip()
    {
        QVersionNumber ver = GNTool::readVersion(m_executable);
        if (ver.isNull())
            m_tooltip = Tr::tr("Cannot get tool version.");
        else
            m_tooltip = Tr::tr("Version: %1").arg(ver.toString());
    }

    QString m_name;
    QString m_tooltip;
    FilePath m_executable;
    Id m_id;
    bool m_autoDetected = false;
    bool m_pathExists = false;
    bool m_pathIsFile = false;
    bool m_pathIsExecutable = false;
    bool m_unsavedChanges = false;
};

// GNToolsModel

class GNToolsModel final : public TreeModel<TreeItem, TreeItem, GNToolTreeItem>
{
    Q_OBJECT

public:
    GNToolsModel()
    {
        setHeader({Tr::tr("Name"), Tr::tr("Location")});
        rootItem()->appendChild(
            new StaticTreeItem({ProjectExplorer::Constants::msgAutoDetected()},
                               {ProjectExplorer::Constants::msgAutoDetectedToolTip()}));
        rootItem()->appendChild(new StaticTreeItem(ProjectExplorer::Constants::msgManual()));
        for (const auto &tool : GNTools::tools())
            addToolHelper(tool);
    }

    GNToolTreeItem *gnToolTreeItem(const QModelIndex &index) const
    {
        return itemForIndexAtLevel<2>(index);
    }

    void updateItem(const Id &itemId, const QString &name, const FilePath &exe)
    {
        auto treeItem = findItemAtLevel<2>(
            [itemId](GNToolTreeItem *n) { return n->id() == itemId; });
        QTC_ASSERT(treeItem, return);
        treeItem->update(name, exe);
    }

    void addGNTool()
    {
        manualGroup()->appendChild(new GNToolTreeItem{uniqueName(Tr::tr("New GN"))});
        markSettingsDirty();
    }

    void removeGNTool(GNToolTreeItem *item)
    {
        QTC_ASSERT(item, return);
        const Id id = item->id();
        destroyItem(item);
        m_itemsToRemove.enqueue(id);
        markSettingsDirty();
    }

    GNToolTreeItem *cloneGNTool(GNToolTreeItem *item)
    {
        QTC_ASSERT(item, return nullptr);
        auto newItem = new GNToolTreeItem(*item);
        manualGroup()->appendChild(newItem);
        markSettingsDirty();
        return newItem;
    }

    void apply()
    {
        forItemsAtLevel<2>([this](GNToolTreeItem *item) {
            if (item->hasUnsavedChanges()) {
                GNTools::updateTool(item->id(), item->name(), item->executable());
                item->setSaved();
                emit this->dataChanged(item->index(), item->index());
            }
        });
        while (!m_itemsToRemove.isEmpty())
            GNTools::removeTool(m_itemsToRemove.dequeue());
    }

private:
    void addToolHelper(const GNTools::Tool_t &tool)
    {
        if (tool->autoDetected())
            autoDetectedGroup()->appendChild(new GNToolTreeItem(tool));
        else
            manualGroup()->appendChild(new GNToolTreeItem(tool));
    }

    QString uniqueName(const QString &baseName)
    {
        QStringList names;
        forItemsAtLevel<2>([&names](GNToolTreeItem *item) { names << item->name(); });
        return Utils::makeUniquelyNumbered(baseName, names);
    }

    TreeItem *autoDetectedGroup() const { return rootItem()->childAt(0); }
    TreeItem *manualGroup() const { return rootItem()->childAt(1); }

    QQueue<Id> m_itemsToRemove;
};

// ToolItemSettings widget

class GNToolItemSettings final : public QWidget
{
    Q_OBJECT

public:
    GNToolItemSettings()
    {
        m_nameLineEdit = new QLineEdit;

        m_pathChooser = new PathChooser;
        m_pathChooser->setExpectedKind(PathChooser::ExistingCommand);
        m_pathChooser->setHistoryCompleter("GN.Command.History");

        using namespace Layouting;

        Form{Tr::tr("Name:"), m_nameLineEdit, br, Tr::tr("Path:"), m_pathChooser, br, noMargin}.attachTo(
            this);

        connect(m_pathChooser, &PathChooser::rawPathChanged, this, &GNToolItemSettings::store);
        connect(m_nameLineEdit, &QLineEdit::textChanged, this, &GNToolItemSettings::store);
    }

    void load(GNToolTreeItem *item)
    {
        if (item) {
            m_currentId = std::nullopt;
            m_nameLineEdit->setDisabled(item->isAutoDetected());
            m_nameLineEdit->setText(item->name());
            m_pathChooser->setDisabled(item->isAutoDetected());
            m_pathChooser->setFilePath(item->executable());
            m_currentId = item->id();
        } else {
            m_currentId = std::nullopt;
        }
    }

    void store()
    {
        if (m_currentId)
            emit applyChanges(*m_currentId, m_nameLineEdit->text(), m_pathChooser->filePath());
    }

signals:
    void applyChanges(Id itemId, const QString &name, const FilePath &exe);

private:
    std::optional<Id> m_currentId;
    QLineEdit *m_nameLineEdit;
    PathChooser *m_pathChooser;
};

// GNToolsSettingsWidget

class GNToolsSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_OBJECT

public:
    GNToolsSettingsWidget()
    {
        m_gnList = new QTreeView;
        m_gnList->setModel(&m_model);
        m_gnList->expandAll();
        m_gnList->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_gnList->header()->setSectionResizeMode(1, QHeaderView::Stretch);

        m_itemSettings = new GNToolItemSettings;

        m_gnDetails = new DetailsWidget;
        m_gnDetails->setState(DetailsWidget::NoSummary);
        m_gnDetails->setVisible(false);
        m_gnDetails->setWidget(m_itemSettings);

        auto addButton = new QPushButton(Tr::tr("Add"));

        m_cloneButton = new QPushButton(Tr::tr("Clone"));
        m_cloneButton->setEnabled(false);

        m_removeButton = new QPushButton(Tr::tr("Remove"));
        m_removeButton->setEnabled(false);

        using namespace Layouting;

        Row{Column{m_gnList, m_gnDetails}, Column{addButton, m_cloneButton, m_removeButton, st}}.attachTo(
            this);

        connect(m_gnList->selectionModel(),
                &QItemSelectionModel::currentChanged,
                this,
                &GNToolsSettingsWidget::currentToolChanged);
        connect(m_itemSettings,
                &GNToolItemSettings::applyChanges,
                &m_model,
                &GNToolsModel::updateItem);

        connect(addButton, &QPushButton::clicked, &m_model, &GNToolsModel::addGNTool);
        connect(m_cloneButton, &QPushButton::clicked, this, &GNToolsSettingsWidget::cloneGNTool);
        connect(m_removeButton, &QPushButton::clicked, this, &GNToolsSettingsWidget::removeGNTool);

        installMarkSettingsDirtyTriggerRecursively(this);
    }

private:
    void apply() final { m_model.apply(); }

    void cloneGNTool()
    {
        if (m_currentItem) {
            auto newItem = m_model.cloneGNTool(m_currentItem);
            m_gnList->setCurrentIndex(newItem->index());
        }
    }

    void removeGNTool()
    {
        if (m_currentItem)
            m_model.removeGNTool(m_currentItem);
    }

    void currentToolChanged(
        const QModelIndex &newCurrent)
    {
        m_currentItem = m_model.gnToolTreeItem(newCurrent);
        m_itemSettings->load(m_currentItem);
        m_gnDetails->setVisible(m_currentItem);
        m_cloneButton->setEnabled(m_currentItem);
        m_removeButton->setEnabled(m_currentItem && !m_currentItem->isAutoDetected());
    }

    GNToolsModel m_model;
    GNToolItemSettings *m_itemSettings = nullptr;
    GNToolTreeItem *m_currentItem = nullptr;

    QTreeView *m_gnList = nullptr;
    DetailsWidget *m_gnDetails = nullptr;
    QPushButton *m_cloneButton = nullptr;
    QPushButton *m_removeButton = nullptr;
};

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

#include "gntoolssettingspage.moc"
