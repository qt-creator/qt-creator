// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakesettingspage.h"

#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmaketool.h"
#include "cmaketoolmanager.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/devicemanagermodel.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/headerviewstretcher.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QComboBox>
#include <QHeaderView>
#include <QPushButton>
#include <QTreeView>

using namespace Utils;
using namespace ProjectExplorer;

namespace CMakeProjectManager::Internal {

//
// CMakeToolItemModel
//

class CMakeToolItemModel : public TreeModel<TreeItem, TreeItem, CMakeToolTreeItem>
{
public:
    CMakeToolItemModel();

    CMakeToolTreeItem *cmakeToolItem(const Utils::Id &id) const;
    CMakeToolTreeItem *cmakeToolItem(const QModelIndex &index) const;
    QModelIndex addCMakeTool(
        const QString &name,
        const FilePath &executable,
        const FilePath &qchFile,
        const bool autoRun,
        const DetectionSource &detectionSource);
    void addCMakeTool(const CMakeTool *item, bool changed);
    TreeItem *autoGroupItem() const;
    TreeItem *manualGroupItem() const;
    void reevaluateChangedFlag(CMakeToolTreeItem *item) const;
    void updateCMakeTool(const Utils::Id &id,
                         const QString &displayName,
                         const FilePath &executable,
                         const FilePath &qchFile);
    void removeCMakeTool(const Utils::Id &id);
    void apply();

    Utils::Id defaultItemId() const;
    void setDefaultItemId(const Utils::Id &id);

    QString uniqueDisplayName(const QString &base) const;
private:
    QVariant data(const QModelIndex &index, int role) const override;

    Utils::Id m_defaultItemId;
    QList<Utils::Id> m_removedItems;
};

CMakeToolTreeItem::CMakeToolTreeItem(Id specialId)
    : m_id(specialId)
{}

CMakeToolTreeItem::CMakeToolTreeItem(const CMakeTool *item, bool changed)
    : m_id(item->id())
    , m_name(item->displayName())
    , m_executable(item->filePath())
    , m_qchFile(item->qchFilePath())
    , m_versionDisplay(item->versionDisplay())
    , m_detectionSource(item->detectionSource())
    , m_isSupported(item->hasFileApi())
    , m_changed(changed)
{
    updateErrorFlags();
}

CMakeToolTreeItem::CMakeToolTreeItem(
    const QString &name,
    const FilePath &executable,
    const FilePath &qchFile,
    bool autoRun,
    const DetectionSource &detectionSource)
    : m_id(Id::generate())
    , m_name(name)
    , m_executable(executable)
    , m_qchFile(qchFile)
    , m_detectionSource(detectionSource)
    , m_isAutoRun(autoRun)
{
    updateErrorFlags();
}

void CMakeToolTreeItem::updateErrorFlags()
{
    const FilePath filePath = CMakeTool::cmakeExecutable(m_executable);
    m_pathExists = filePath.exists();
    m_pathIsFile = filePath.isFile();
    m_pathIsExecutable = filePath.isExecutableFile();

    std::unique_ptr<CMakeTool> temporaryTool;
    CMakeTool *cmake = CMakeToolManager::findById(m_id);
    if (!cmake) {
        temporaryTool = std::make_unique<CMakeTool>(m_detectionSource, m_id);
        cmake = temporaryTool.get();
        cmake->setFilePath(m_executable);
    }
    cmake->setFilePath(m_executable);
    m_isSupported = cmake->hasFileApi();

    m_tooltip = Tr::tr("Version: %1").arg(cmake->versionDisplay());
    m_tooltip += "<br>"
                 + Tr::tr("Supports fileApi: %1").arg(m_isSupported ? Tr::tr("yes") : Tr::tr("no"));
    m_tooltip += "<br>" + Tr::tr("Detection source: \"%1\"").arg(m_detectionSource.id);

    m_versionDisplay = cmake->versionDisplay();

    // Make sure to always have the right version in the name for Qt SDK CMake installations
    if (m_detectionSource.isAutoDetected() && m_name.startsWith("CMake") && m_name.endsWith("(Qt)"))
        m_name = QString("CMake %1 (Qt)").arg(m_versionDisplay);
}

bool CMakeToolTreeItem::hasError() const
{
    return !m_isSupported || !m_pathExists || !m_pathIsFile || !m_pathIsExecutable;
}

QVariant CMakeToolTreeItem::data(int column, int role) const
{
    const auto defaultCMake = [this] {
        return Id::fromSetting(model()->data({}, DefaultExecutableRole));
    };

    if (!m_id.isValid()) {
        if (role == Qt::DisplayRole && column == 0)
            return Tr::tr("None", "No CMake tool");
        if (role == ProjectExplorer::KitAspect::IsNoneRole)
            return true;
        return {};
    }

    switch (role) {
    case FilePathRole: return m_executable.toVariant();
    case Qt::DisplayRole: {
        switch (column) {
        case 0: {
            QString name = m_name;
            if (defaultCMake() == m_id)
                name += Tr::tr(" (Default)");
            return name;
        }
        case 1: {
            return m_executable.toUserOutput();
        }
        } // switch (column)
        return QVariant();
    }
    case Qt::FontRole: {
        QFont font;
        font.setBold(m_changed);
        font.setItalic(defaultCMake() == m_id);
        return font;
    }
    case Qt::ToolTipRole: {
        QString result = m_tooltip;
        QString error;
        if (!m_pathExists) {
            error = Tr::tr("CMake executable path does not exist.");
        } else if (!m_pathIsFile) {
            error = Tr::tr("CMake executable path is not a file.");
        } else if (!m_pathIsExecutable) {
            error = Tr::tr("CMake executable path is not executable.");
        } else if (!m_isSupported) {
            error = Tr::tr("CMake executable does not provide required IDE integration features.");
        }
        if (result.isEmpty() || error.isEmpty())
            return QString("%1%2").arg(result).arg(error);
        else
            return QString("%1<br><br><b>%2</b>").arg(result).arg(error);
    }
    case Qt::DecorationRole: {
        if (column == 0 && hasError())
            return Icons::CRITICAL.icon();
        return QVariant();
    }
    case ProjectExplorer::KitAspect::IdRole:
        return m_id.toSetting();
    case ProjectExplorer::KitAspect::QualityRole:
        return int(!hasError());
    }
    return QVariant();
}

CMakeToolItemModel::CMakeToolItemModel()
{
    setHeader({Tr::tr("Name"), Tr::tr("Path")});
    rootItem()->appendChild(
        new StaticTreeItem({ProjectExplorer::Constants::msgAutoDetected()},
                           {ProjectExplorer::Constants::msgAutoDetectedToolTip()}));
    rootItem()->appendChild(new StaticTreeItem(Tr::tr("Manual")));

    const QList<CMakeTool *> items = CMakeToolManager::cmakeTools();
    for (const CMakeTool *item : items)
        addCMakeTool(item, false);

    CMakeTool *defTool = CMakeToolManager::defaultCMakeTool();
    m_defaultItemId = defTool ? defTool->id() : Id();
    connect(CMakeToolManager::instance(), &CMakeToolManager::cmakeRemoved,
            this, &CMakeToolItemModel::removeCMakeTool);
    connect(CMakeToolManager::instance(), &CMakeToolManager::cmakeAdded,
            this, [this](const Id &id) { addCMakeTool(CMakeToolManager::findById(id), false); });

}

QModelIndex CMakeToolItemModel::addCMakeTool(
    const QString &name,
    const FilePath &executable,
    const FilePath &qchFile,
    const bool autoRun,
    const DetectionSource &detectionSource)
{
    auto item = new CMakeToolTreeItem(name, executable, qchFile, autoRun, detectionSource);
    if (detectionSource.isAutoDetected())
        autoGroupItem()->appendChild(item);
    else
        manualGroupItem()->appendChild(item);

    return item->index();
}

void CMakeToolItemModel::addCMakeTool(const CMakeTool *item, bool changed)
{
    QTC_ASSERT(item, return);

    if (cmakeToolItem(item->id()))
        return;

    auto treeItem = new CMakeToolTreeItem(item, changed);
    if (item->detectionSource().isAutoDetected())
        autoGroupItem()->appendChild(treeItem);
    else
        manualGroupItem()->appendChild(treeItem);
}

TreeItem *CMakeToolItemModel::autoGroupItem() const
{
    return rootItem()->childAt(0);
}

TreeItem *CMakeToolItemModel::manualGroupItem() const
{
    return rootItem()->childAt(1);
}

void CMakeToolItemModel::reevaluateChangedFlag(CMakeToolTreeItem *item) const
{
    CMakeTool *orig = CMakeToolManager::findById(item->m_id);
    item->m_changed = !orig || orig->displayName() != item->m_name
                      || orig->filePath() != item->m_executable
                      || orig->qchFilePath() != item->m_qchFile;

    //make sure the item is marked as changed when the default cmake was changed
    CMakeTool *origDefTool = CMakeToolManager::defaultCMakeTool();
    Id origDefault = origDefTool ? origDefTool->id() : Id();
    if (origDefault != m_defaultItemId) {
        if (item->m_id == origDefault || item->m_id == m_defaultItemId)
            item->m_changed = true;
    }

    item->update(); // Notify views.
}

void CMakeToolItemModel::updateCMakeTool(const Id &id,
                                         const QString &displayName,
                                         const FilePath &executable,
                                         const FilePath &qchFile)
{
    CMakeToolTreeItem *treeItem = cmakeToolItem(id);
    QTC_ASSERT(treeItem, return );

    treeItem->m_name = displayName;
    treeItem->m_executable = executable;
    treeItem->m_qchFile = qchFile;

    treeItem->updateErrorFlags();

    reevaluateChangedFlag(treeItem);
}

CMakeToolTreeItem *CMakeToolItemModel::cmakeToolItem(const Id &id) const
{
    return findItemAtLevel<2>([id](CMakeToolTreeItem *n) { return n->m_id == id; });
}

CMakeToolTreeItem *CMakeToolItemModel::cmakeToolItem(const QModelIndex &index) const
{
    return itemForIndexAtLevel<2>(index);
}

void CMakeToolItemModel::removeCMakeTool(const Id &id)
{
    if (m_removedItems.contains(id))
        return; // Item has already been removed in the model!

    CMakeToolTreeItem *treeItem = cmakeToolItem(id);
    QTC_ASSERT(treeItem, return);

    m_removedItems.append(id);
    destroyItem(treeItem);
}

void CMakeToolItemModel::apply()
{
    for (const Id &id : std::as_const(m_removedItems))
        CMakeToolManager::deregisterCMakeTool(id);

    QList<CMakeToolTreeItem *> toRegister;
    forItemsAtLevel<2>([&toRegister](CMakeToolTreeItem *item) {
        item->m_changed = false;
        if (CMakeTool *cmake = CMakeToolManager::findById(item->m_id)) {
            cmake->setDisplayName(item->m_name);
            cmake->setFilePath(item->m_executable);
            cmake->setQchFilePath(item->m_qchFile);
            cmake->setDetectionSource(item->m_detectionSource);
        } else {
            toRegister.append(item);
        }
    });

    for (CMakeToolTreeItem *item : std::as_const(toRegister)) {
        auto cmake = std::make_unique<CMakeTool>(item->m_detectionSource, item->m_id);
        cmake->setDisplayName(item->m_name);
        cmake->setFilePath(item->m_executable);
        cmake->setQchFilePath(item->m_qchFile);
        if (!CMakeToolManager::registerCMakeTool(std::move(cmake)))
            item->m_changed = true;
    }

    CMakeToolManager::setDefaultCMakeTool(defaultItemId());
}

Id CMakeToolItemModel::defaultItemId() const
{
    return m_defaultItemId;
}

void CMakeToolItemModel::setDefaultItemId(const Id &id)
{
    if (m_defaultItemId == id)
        return;

    Id oldDefaultId = m_defaultItemId;
    m_defaultItemId = id;

    CMakeToolTreeItem *newDefault = cmakeToolItem(id);
    if (newDefault)
        reevaluateChangedFlag(newDefault);

    CMakeToolTreeItem *oldDefault = cmakeToolItem(oldDefaultId);
    if (oldDefault)
        reevaluateChangedFlag(oldDefault);
}


QString CMakeToolItemModel::uniqueDisplayName(const QString &base) const
{
    QStringList names;
    forItemsAtLevel<2>([&names](CMakeToolTreeItem *item) { names << item->m_name; });
    return Utils::makeUniquelyNumbered(base, names);
}

QVariant CMakeToolItemModel::data(const QModelIndex &index, int role) const
{
    if (role == CMakeToolTreeItem::DefaultExecutableRole)
        return defaultItemId().toSetting();
    return TreeModel::data(index, role);
}

//
// CMakeToolItemConfigWidget
//

class CMakeToolItemConfigData final : public AspectContainer
{
public:
    CMakeToolItemConfigData()
    {
        displayName.setDisplayStyle(StringAspect::LineEditDisplay);
        displayName.setLabelText(Tr::tr("Name:"));

        binary.setExpectedKind(PathChooser::ExistingCommand);
        binary.setHistoryCompleter("Cmake.Command.History");
        binary.setCommandVersionArguments({"--version"});
        binary.setAllowPathFromDevice(true);
        binary.setLabelText(Tr::tr("Path:"));

        version.setDisplayStyle(StringAspect::LabelDisplay);
        version.setLabelText(Tr::tr("Version:"));

        qchFile.setExpectedKind(PathChooser::File);
        qchFile.setHistoryCompleter("Cmake.qchFile.History");
        qchFile.setPromptDialogFilter("*.qch");
        qchFile.setPromptDialogTitle(Tr::tr("CMake .qch File"));
        qchFile.setLabelText(Tr::tr("Help file:"));
    }

    StringAspect displayName{this};
    FilePathAspect binary{this};
    StringAspect version{this};
    FilePathAspect qchFile{this};
};

class CMakeToolItemConfigWidget : public QWidget
{
public:
    explicit CMakeToolItemConfigWidget(CMakeToolItemModel *model);
    void load(const CMakeToolTreeItem *item, bool updateCMakePath);
    void setDeviceRoot(const FilePath &devRoot);
    void store() const;

private:
    void onBinaryPathEditingFinished();
    void updateQchFilePath();

    CMakeToolItemModel *m_model;
    CMakeToolItemConfigData m_data;
    Id m_id;
    bool m_loadingItem = false;
};

CMakeToolItemConfigWidget::CMakeToolItemConfigWidget(CMakeToolItemModel *model)
    : m_model(model)
{
    using namespace Layouting;
    Form {
        m_data.displayName, br,
        m_data.binary, br,
        m_data.version, br,
        m_data.qchFile, br,
        noMargin,
    }.attachTo(this);

    m_data.binary.addOnVolatileValueChanged(this, [this] { onBinaryPathEditingFinished(); });
    m_data.qchFile.addOnVolatileValueChanged(this, [this] { store(); });
    m_data.displayName.addOnVolatileValueChanged(this, [this] { store(); });
}

void CMakeToolItemConfigWidget::store() const
{
    if (!m_loadingItem && m_id.isValid()) {
        m_model->updateCMakeTool(m_id,
                                 m_data.displayName.volatileValue(),
                                 FilePath::fromUserInput(m_data.binary.volatileValue()),
                                 FilePath::fromUserInput(m_data.qchFile.volatileValue()));
    }
}

void CMakeToolItemConfigWidget::onBinaryPathEditingFinished()
{
    updateQchFilePath();
    store();
    load(m_model->cmakeToolItem(m_id), false);
}

void CMakeToolItemConfigWidget::updateQchFilePath()
{
    if (m_data.qchFile().isEmpty()) {
        m_data.qchFile.setValue(
            CMakeTool::searchQchFile(FilePath::fromUserInput(m_data.binary.volatileValue())));
    }
}

void CMakeToolItemConfigWidget::load(const CMakeToolTreeItem *item, bool updateCMakePath)
{
    m_loadingItem = true; // avoid intermediate signal handling
    m_id = Id();
    if (!item) {
        m_loadingItem = false;
        return;
    }

    // Set values:
    m_data.displayName.setEnabled(!item->m_detectionSource.isAutoDetected());
    m_data.displayName.setValue(item->m_name);

    m_data.binary.setReadOnly(item->m_detectionSource.isAutoDetected());
    if (updateCMakePath)
        m_data.binary.setValue(item->m_executable);

    m_data.qchFile.setReadOnly(item->m_detectionSource.isAutoDetected());
    m_data.qchFile.setBaseDirectory(item->m_executable.parentDir());
    m_data.qchFile.setValue(item->m_qchFile);

    m_data.version.setValue(item->m_versionDisplay);

    m_id = item->m_id;
    m_loadingItem = false;
}

void CMakeToolItemConfigWidget::setDeviceRoot(const FilePath &devRoot)
{
    m_data.binary.setInitialBrowsePathBackup(devRoot);
}

//
// CMakeToolConfigWidget
//

class CMakeToolConfigWidget : public Core::IOptionsPageWidget
{
public:
    CMakeToolConfigWidget()
    {
        m_addButton.setText(Tr::tr("Add"));

        m_cloneButton.setText(Tr::tr("Clone"));
        m_cloneButton.setEnabled(false);

        m_delButton.setText(Tr::tr("Remove"));
        m_delButton.setEnabled(false);

        m_makeDefButton.setText(Tr::tr("Make Default"));
        m_makeDefButton.setEnabled(false);
        m_makeDefButton.setToolTip(Tr::tr("Set as the default CMake Tool to use when "
                                          "creating a new kit or when no value is set."));

        m_detectButton.setText(Tr::tr("Re-detect"));

        m_container.setState(DetailsWidget::NoSummary);
        m_container.setVisible(false);
        m_container.setWidget(&m_itemConfigWidget);

        m_deviceModel.showAllEntry();
        m_deviceComboBox.setModel(&m_deviceModel);

        m_filterModel.setSourceModel(&m_model);
        m_cmakeToolsView.setModel(&m_filterModel);
        m_cmakeToolsView.setUniformRowHeights(true);
        m_cmakeToolsView.setSelectionMode(QAbstractItemView::SingleSelection);
        m_cmakeToolsView.setSelectionBehavior(QAbstractItemView::SelectRows);
        m_cmakeToolsView.expandAll();

        QHeaderView *header = m_cmakeToolsView.header();
        header->setStretchLastSection(false);
        header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(1, QHeaderView::Stretch);
        (void) new HeaderViewStretcher(header, 0);

        using namespace Layouting;
        Column {
            Row { Tr::tr("Device:"), m_deviceComboBox, st },
            Row {
                Column {
                    m_cmakeToolsView,
                    m_container,
                },
                Column {
                    m_addButton,
                    m_cloneButton,
                    m_delButton,
                    m_makeDefButton,
                    m_detectButton,
                    st,
                },
            }}.attachTo(this);

        connect(m_cmakeToolsView.selectionModel(), &QItemSelectionModel::currentChanged,
                this, &CMakeToolConfigWidget::currentCMakeToolChanged, Qt::QueuedConnection);

        connect(&m_addButton, &QAbstractButton::clicked,
                this, &CMakeToolConfigWidget::addCMakeTool);
        connect(&m_cloneButton, &QAbstractButton::clicked,
                this, &CMakeToolConfigWidget::cloneCMakeTool);
        connect(&m_delButton, &QAbstractButton::clicked,
                this, &CMakeToolConfigWidget::removeCMakeTool);
        connect(&m_makeDefButton, &QAbstractButton::clicked,
                this, &CMakeToolConfigWidget::setDefaultCMakeTool);
        connect(&m_detectButton, &QAbstractButton::clicked,
                this, &CMakeToolConfigWidget::redetect);

        const auto updateDevice = [this](int idx) {
            m_filterModel.setDevice(m_deviceModel.device(idx));
        };
        connect(&m_deviceComboBox, &QComboBox::currentIndexChanged, this, updateDevice);
        m_deviceComboBox.setCurrentIndex(0);
        updateDevice(m_deviceComboBox.currentIndex());

        setIgnoreForDirtyHook(&m_deviceComboBox);
        installMarkSettingsDirtyTriggerRecursively(this);
    }

private:
    void apply() final;

    void cloneCMakeTool();
    void addCMakeTool();
    void removeCMakeTool();
    void setDefaultCMakeTool();
    void redetect();
    void currentCMakeToolChanged(const QModelIndex &newCurrent);

    QModelIndex mapFromSource(const QModelIndex &idx) const;
    QModelIndex mapToSource(const QModelIndex &idx) const;
    IDeviceConstPtr currentDevice() const;

    CMakeToolItemModel m_model;
    DeviceManagerModel m_deviceModel;
    DeviceFilterModel m_filterModel;
    QComboBox m_deviceComboBox;
    QTreeView m_cmakeToolsView;
    QPushButton m_addButton;
    QPushButton m_cloneButton;
    QPushButton m_delButton;
    QPushButton m_makeDefButton;
    QPushButton m_detectButton;
    DetailsWidget m_container;
    CMakeToolItemConfigWidget m_itemConfigWidget{&m_model};
    CMakeToolTreeItem *m_currentItem = nullptr;
};

void CMakeToolConfigWidget::apply()
{
    m_itemConfigWidget.store();
    m_model.apply();
}

void CMakeToolConfigWidget::cloneCMakeTool()
{
    if (!m_currentItem)
        return;

    QModelIndex newItem = m_model.addCMakeTool(
        Tr::tr("Clone of %1").arg(m_currentItem->m_name),
        m_currentItem->m_executable,
        m_currentItem->m_qchFile,
        m_currentItem->m_isAutoRun,
        DetectionSource{DetectionSource::Manual, m_currentItem->m_detectionSource.id});

    m_cmakeToolsView.setCurrentIndex(mapFromSource(newItem));
    markSettingsDirty();
}

void CMakeToolConfigWidget::addCMakeTool()
{
    QModelIndex newItem = m_model.addCMakeTool(
        m_model.uniqueDisplayName(Tr::tr("New CMake")),
        FilePath(),
        FilePath(),
        true,
        DetectionSource::Manual);

    m_cmakeToolsView.setCurrentIndex(mapFromSource(newItem));
    markSettingsDirty();
}

void CMakeToolConfigWidget::removeCMakeTool()
{
    bool delDef = m_model.defaultItemId() == m_currentItem->m_id;
    m_model.removeCMakeTool(m_currentItem->m_id);
    m_currentItem = nullptr;
    markSettingsDirty();

    if (delDef) {
        auto it = static_cast<CMakeToolTreeItem *>(m_model.autoGroupItem()->firstChild());
        if (!it)
            it = static_cast<CMakeToolTreeItem *>(m_model.manualGroupItem()->firstChild());
        if (it)
            m_model.setDefaultItemId(it->m_id);
    }

    TreeItem *newCurrent = m_model.manualGroupItem()->lastChild();
    if (!newCurrent)
        newCurrent = m_model.autoGroupItem()->lastChild();

    if (newCurrent)
        m_cmakeToolsView.setCurrentIndex(newCurrent->index());
}

void CMakeToolConfigWidget::setDefaultCMakeTool()
{
    if (!m_currentItem)
        return;

    m_model.setDefaultItemId(m_currentItem->m_id);
    m_makeDefButton.setEnabled(false);
    markSettingsDirty();
}

void CMakeToolConfigWidget::redetect()
{
    // Step 1: Detect
    QList<IDeviceConstPtr> devices;
    if (const IDeviceConstPtr dev = currentDevice()) {
        devices << dev;
    } else {
        for (int i = 0; i < DeviceManager::deviceCount(); ++i)
            devices << DeviceManager::deviceAt(i);
    }
    std::vector<std::unique_ptr<CMakeTool>> toAdd;
    for (const IDeviceConstPtr &dev : std::as_const(devices)) {
        auto detected
            = CMakeToolManager::autoDetectCMakeTools(dev->toolSearchPaths(), dev->rootPath());
        for (auto &&tool : detected)
            toAdd.push_back(std::move(tool));
    }

    // Step 2: Match existing against newly detected.
    QList<Id> toRemove;
    m_model.forItemsAtLevel<2>([&](const CMakeToolTreeItem *item) {
        bool hasMatch = false;
        for (auto it = toAdd.begin(); it != toAdd.end(); ++it) {
            if ((*it)->filePath().isSameFile(item->executable())) {
                hasMatch = true;
                toAdd.erase(it);
                break;
            }
        }
        if (item->detectionSource().isSystemDetected() && !hasMatch)
            toRemove << item->id();
    });

    // Step 3: Remove previously auto-detected that were not newly detected.
    for (const Id &id : std::as_const(toRemove))
        m_model.removeCMakeTool(id);

    // Step 4: Add newly detected that haven't been present so far.
    for (const auto &tool : toAdd)
        m_model.addCMakeTool(tool.get(), true);

    if (!toRemove.isEmpty() || !toAdd.empty())
        markSettingsDirty();
}

void CMakeToolConfigWidget::currentCMakeToolChanged(const QModelIndex &newCurrent)
{
    m_currentItem = m_model.cmakeToolItem(mapToSource(newCurrent));
    m_itemConfigWidget.load(m_currentItem, true);
    if (const IDeviceConstPtr dev = currentDevice())
        m_itemConfigWidget.setDeviceRoot(dev->rootPath());
    m_container.setVisible(m_currentItem);
    m_cloneButton.setEnabled(m_currentItem);
    m_delButton.setEnabled(m_currentItem && !m_currentItem->m_detectionSource.isAutoDetected());
    m_makeDefButton.setEnabled(m_currentItem && (!m_model.defaultItemId().isValid() || m_currentItem->m_id != m_model.defaultItemId()));
}

QModelIndex CMakeToolConfigWidget::mapFromSource(const QModelIndex &idx) const
{
    QTC_ASSERT(m_filterModel.sourceModel() == &m_model, return {});
    return m_filterModel.mapFromSource(idx);
}

QModelIndex CMakeToolConfigWidget::mapToSource(const QModelIndex &idx) const
{
    QTC_ASSERT(m_filterModel.sourceModel() == &m_model, return {});
    return m_filterModel.mapToSource(idx);
}

IDeviceConstPtr CMakeToolConfigWidget::currentDevice() const
{
    return m_deviceModel.device(m_deviceComboBox.currentIndex());
}

// CMakeSettingsPage

class CMakeSettingsPage final : public Core::IOptionsPage
{
public:
    CMakeSettingsPage()
    {
        setId(Constants::Settings::TOOLS_ID);
        setDisplayName(Tr::tr("Tools"));
        setCategory(Constants::Settings::CATEGORY);
        setWidgetCreator([] { return new CMakeToolConfigWidget; });
    }
};

void setupCMakeSettingsPage()
{
    static CMakeSettingsPage theCMakeSettingsPage;
}

} // CMakeProjectManager::Internal
