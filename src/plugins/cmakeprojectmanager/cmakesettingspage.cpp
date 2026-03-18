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
#include <utils/groupedmodel.h>
#include <utils/headerviewstretcher.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/treemodel.h> // for FilePathRole
#include <utils/utilsicons.h>

#include <QComboBox>
#include <QHeaderView>
#include <QPushButton>
#include <QTreeView>

using namespace Utils;
using namespace ProjectExplorer;

namespace CMakeProjectManager::Internal {

class CMakeToolTreeItem
{
public:
    CMakeToolTreeItem() = default;
    CMakeToolTreeItem(const CMakeTool *item, bool changed);
    CMakeToolTreeItem(
        const QString &name,
        const FilePath &executable,
        const FilePath &qchFile,
        bool autoRun,
        const DetectionSource &detectionSource);

    void updateErrorFlags();
    bool hasError() const;

    Id id() const { return m_id; }
    FilePath executable() const { return m_executable; }
    DetectionSource detectionSource() const { return m_detectionSource; }

    QVariant data(int column, int role) const;

    friend bool operator==(const CMakeToolTreeItem &a, const CMakeToolTreeItem &b)
    {
        return a.m_id == b.m_id
            && a.m_name == b.m_name
            && a.m_executable == b.m_executable
            && a.m_qchFile == b.m_qchFile
            && a.m_detectionSource == b.m_detectionSource
            && a.m_isAutoRun == b.m_isAutoRun
            && a.m_changed == b.m_changed;
    }

    Id m_id;
    QString m_name;
    QString m_tooltip;
    FilePath m_executable;
    FilePath m_qchFile;
    QString m_versionDisplay;
    DetectionSource m_detectionSource;
    bool m_isAutoRun = true;
    bool m_pathExists = false;
    bool m_pathIsFile = false;
    bool m_pathIsExecutable = false;
    bool m_isSupported = false;
    bool m_changed = true;
    bool m_isDefault = false;
};

} // namespace CMakeProjectManager::Internal

Q_DECLARE_METATYPE(CMakeProjectManager::Internal::CMakeToolTreeItem)

namespace CMakeProjectManager::Internal {

//
// CMakeToolItemModel
//

class CMakeToolItemModel final : public TypedGroupedModel<CMakeToolTreeItem>
{
public:
    CMakeToolItemModel();

    QModelIndex addCMakeTool(
        const QString &name,
        const FilePath &executable,
        const FilePath &qchFile,
        bool autoRun,
        const DetectionSource &detectionSource);
    void addCMakeTool(const CMakeTool *item, bool changed);
    void reevaluateChangedFlag(int row);
    void updateCMakeTool(const Id &id,
                         const QString &displayName,
                         const FilePath &executable,
                         const FilePath &qchFile);
    void removeCMakeTool(const Id &id);
    void apply() final;

    Id defaultItemId() const;
    void setDefaultItemId(const Id &id);

    QString uniqueDisplayName(const QString &base) const;
    int rowForId(Id id) const;

private:
    QVariant variantData(const QVariant &v, int column, int role) const override
    {
        return fromVariant(v).data(column, role);
    }

    Id m_defaultItemId;
};

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
    switch (role) {
    case FilePathRole: return m_executable.toVariant();
    case Qt::DisplayRole: {
        switch (column) {
        case 0: {
            QString name = m_name;
            if (m_isDefault)
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
        font.setItalic(m_isDefault);
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
    setFilters(ProjectExplorer::Constants::msgAutoDetected(),
               {{Tr::tr("Manual"), [this](int row) {
                   return !item(row).m_detectionSource.isAutoDetected();
               }}});

    const QList<CMakeTool *> tools = CMakeToolManager::cmakeTools();
    for (const CMakeTool *tool : tools)
        addCMakeTool(tool, false);

    CMakeTool *defTool = CMakeToolManager::defaultCMakeTool();
    m_defaultItemId = defTool ? defTool->id() : Id();
    if (const int row = rowForId(m_defaultItemId); row >= 0) {
        CMakeToolTreeItem it = item(row);
        it.m_isDefault = true;
        setVolatileItem(row, it);
    }

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
    CMakeToolTreeItem treeItem(name, executable, qchFile, autoRun, detectionSource);
    treeItem.m_isDefault = (m_defaultItemId == treeItem.m_id);
    return mapFromSource(appendItem(treeItem));
}

void CMakeToolItemModel::addCMakeTool(const CMakeTool *item, bool changed)
{
    QTC_ASSERT(item, return);

    if (rowForId(item->id()) >= 0)
        return;

    CMakeToolTreeItem treeItem(item, changed);
    treeItem.m_isDefault = (m_defaultItemId == treeItem.m_id);
    appendItem(treeItem);
}

void CMakeToolItemModel::reevaluateChangedFlag(int row)
{
    CMakeToolTreeItem it = item(row);
    CMakeTool *orig = CMakeToolManager::findById(it.m_id);
    it.m_changed = !orig || orig->displayName() != it.m_name
                   || orig->filePath() != it.m_executable
                   || orig->qchFilePath() != it.m_qchFile;

    // Make sure the item is marked as changed when the default cmake was changed.
    CMakeTool *origDefTool = CMakeToolManager::defaultCMakeTool();
    Id origDefault = origDefTool ? origDefTool->id() : Id();
    if (origDefault != m_defaultItemId) {
        if (it.m_id == origDefault || it.m_id == m_defaultItemId)
            it.m_changed = true;
    }

    setVolatileItem(row, it);
    notifyRowChanged(row);
}

void CMakeToolItemModel::updateCMakeTool(const Id &id,
                                         const QString &displayName,
                                         const FilePath &executable,
                                         const FilePath &qchFile)
{
    const int row = rowForId(id);
    QTC_ASSERT(row >= 0, return);

    CMakeToolTreeItem it = item(row);
    it.m_name = displayName;
    it.m_executable = executable;
    it.m_qchFile = qchFile;
    it.updateErrorFlags();
    setVolatileItem(row, it);
    reevaluateChangedFlag(row);
}

int CMakeToolItemModel::rowForId(Id id) const
{
    for (int row = 0; row < itemCount(); ++row) {
        if (item(row).m_id == id)
            return row;
    }
    return -1;
}

void CMakeToolItemModel::removeCMakeTool(const Id &id)
{
    // Called via the cmakeRemoved signal: the tool is already gone from the manager.
    const int row = rowForId(id);
    if (row < 0 || isRemoved(row))
        return;

    removeItem(row);
}

void CMakeToolItemModel::apply()
{
    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row))
            CMakeToolManager::deregisterCMakeTool(item(row).m_id);
    }

    QList<int> toRegister;
    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row))
            continue;
        CMakeToolTreeItem it = item(row);
        it.m_changed = false;
        if (CMakeTool *cmake = CMakeToolManager::findById(it.m_id)) {
            cmake->setDisplayName(it.m_name);
            cmake->setFilePath(it.m_executable);
            cmake->setQchFilePath(it.m_qchFile);
            cmake->setDetectionSource(it.m_detectionSource);
        } else {
            toRegister.append(row);
        }
        setVolatileItem(row, it);
    }

    for (int row : std::as_const(toRegister)) {
        const CMakeToolTreeItem it = item(row);
        auto cmake = std::make_unique<CMakeTool>(it.m_detectionSource, it.m_id);
        cmake->setDisplayName(it.m_name);
        cmake->setFilePath(it.m_executable);
        cmake->setQchFilePath(it.m_qchFile);
        if (!CMakeToolManager::registerCMakeTool(std::move(cmake))) {
            CMakeToolTreeItem updated = item(row);
            updated.m_changed = true;
            setVolatileItem(row, updated);
        }
    }

    CMakeToolManager::setDefaultCMakeTool(defaultItemId());
    GroupedModel::apply();
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

    if (const int row = rowForId(id); row >= 0) {
        CMakeToolTreeItem it = item(row);
        it.m_isDefault = true;
        setVolatileItem(row, it);
        reevaluateChangedFlag(row);
    }

    if (const int row = rowForId(oldDefaultId); row >= 0) {
        CMakeToolTreeItem it = item(row);
        it.m_isDefault = false;
        setVolatileItem(row, it);
        reevaluateChangedFlag(row);
    }
}


QString CMakeToolItemModel::uniqueDisplayName(const QString &base) const
{
    QStringList names;
    for (int row = 0; row < itemCount(); ++row)
        names << item(row).m_name;
    return Utils::makeUniquelyNumbered(base, names);
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
    const int row = m_model->rowForId(m_id);
    if (row >= 0) {
        const CMakeToolTreeItem it = m_model->item(row);
        load(&it, false);
    }
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

        m_cmakeToolsView.setModel(m_model.groupedDisplayModel());
        m_cmakeToolsView.setUniformRowHeights(true);
        m_cmakeToolsView.setSelectionMode(QAbstractItemView::SingleSelection);
        m_cmakeToolsView.setSelectionBehavior(QAbstractItemView::SelectRows);
        const auto expandAll = [this] { m_cmakeToolsView.expandAll(); };
        connect(m_model.groupedDisplayModel(), &QAbstractItemModel::modelReset,
                this, expandAll);
        expandAll();

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
            const IDeviceConstPtr device = m_deviceModel.device(idx);
            const FilePath deviceRoot = device ? device->rootPath() : FilePath{};
            m_model.setExtraFilter(deviceRoot.isEmpty()
                ? GroupedModel::Filter{}
                : GroupedModel::Filter{[this, deviceRoot](int row) {
                      const FilePath path = m_model.item(row).m_executable;
                      return path.isEmpty() || path.isSameDevice(deviceRoot);
                  }});
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

    IDeviceConstPtr currentDevice() const;

    CMakeToolItemModel m_model;
    DeviceManagerModel m_deviceModel;
    QComboBox m_deviceComboBox;
    QTreeView m_cmakeToolsView;
    QPushButton m_addButton;
    QPushButton m_cloneButton;
    QPushButton m_delButton;
    QPushButton m_makeDefButton;
    QPushButton m_detectButton;
    DetailsWidget m_container;
    CMakeToolItemConfigWidget m_itemConfigWidget{&m_model};
    Id m_currentItemId;
};

void CMakeToolConfigWidget::apply()
{
    m_itemConfigWidget.store();
    m_model.apply();
}

void CMakeToolConfigWidget::cloneCMakeTool()
{
    const int row = m_model.rowForId(m_currentItemId);
    if (row < 0)
        return;

    const CMakeToolTreeItem it = m_model.item(row);
    QModelIndex newItem = m_model.addCMakeTool(
        Tr::tr("Clone of %1").arg(it.m_name),
        it.m_executable,
        it.m_qchFile,
        it.m_isAutoRun,
        DetectionSource{DetectionSource::Manual, it.m_detectionSource.id});

    m_cmakeToolsView.setCurrentIndex(newItem);
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

    m_cmakeToolsView.setCurrentIndex(newItem);
    markSettingsDirty();
}

void CMakeToolConfigWidget::removeCMakeTool()
{
    const int row = m_model.rowForId(m_currentItemId);
    if (row < 0)
        return;

    const bool wasDefault = m_model.defaultItemId() == m_currentItemId;
    m_model.markRemoved(row);

    if (wasDefault) {
        for (int r = 0; r < m_model.itemCount(); ++r) {
            if (!m_model.isRemoved(r)) {
                m_model.setDefaultItemId(m_model.item(r).m_id);
                break;
            }
        }
    }

    markSettingsDirty();
}

void CMakeToolConfigWidget::setDefaultCMakeTool()
{
    if (!m_currentItemId.isValid())
        return;

    m_model.setDefaultItemId(m_currentItemId);
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
    for (int row = 0; row < m_model.itemCount(); ++row) {
        const CMakeToolTreeItem treeItem = m_model.item(row);
        bool hasMatch = false;
        for (auto it = toAdd.begin(); it != toAdd.end(); ++it) {
            if ((*it)->filePath().isSameFile(treeItem.executable())) {
                hasMatch = true;
                toAdd.erase(it);
                break;
            }
        }
        if (treeItem.detectionSource().isSystemDetected() && !hasMatch)
            toRemove << treeItem.id();
    }

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
    const QModelIndex sourceIdx = m_model.mapToSource(newCurrent);
    const int row = (sourceIdx.isValid() && sourceIdx.row() < m_model.itemCount())
                    ? sourceIdx.row() : -1;
    if (row >= 0) {
        const CMakeToolTreeItem it = m_model.item(row);
        m_currentItemId = it.m_id;
        m_itemConfigWidget.load(&it, true);
        if (const IDeviceConstPtr dev = currentDevice())
            m_itemConfigWidget.setDeviceRoot(dev->rootPath());
        m_delButton.setEnabled(!it.m_detectionSource.isAutoDetected());
        m_makeDefButton.setEnabled(!m_model.defaultItemId().isValid()
                                   || it.m_id != m_model.defaultItemId());
    } else {
        m_currentItemId = {};
        m_itemConfigWidget.load(nullptr, true);
        m_delButton.setEnabled(false);
        m_makeDefButton.setEnabled(false);
    }
    m_container.setVisible(row >= 0);
    m_cloneButton.setEnabled(row >= 0);
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
