/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cmakesettingspage.h"

#include "cmakeprojectconstants.h"
#include "cmaketool.h"
#include "cmaketoolmanager.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/detailswidget.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QCoreApplication>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QTreeView>
#include <QUuid>

using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

class CMakeToolTreeItem;

// --------------------------------------------------------------------------
// CMakeToolItemModel
// --------------------------------------------------------------------------

class CMakeToolItemModel : public TreeModel<TreeItem, TreeItem, CMakeToolTreeItem>
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::CMakeSettingsPage)

public:
    CMakeToolItemModel();

    CMakeToolTreeItem *cmakeToolItem(const Utils::Id &id) const;
    CMakeToolTreeItem *cmakeToolItem(const QModelIndex &index) const;
    QModelIndex addCMakeTool(const QString &name,
                             const FilePath &executable,
                             const FilePath &qchFile,
                             const bool autoRun,
                             const bool autoCreate,
                             const bool isAutoDetected);
    void addCMakeTool(const CMakeTool *item, bool changed);
    TreeItem *autoGroupItem() const;
    TreeItem *manualGroupItem() const;
    void reevaluateChangedFlag(CMakeToolTreeItem *item) const;
    void updateCMakeTool(const Utils::Id &id,
                         const QString &displayName,
                         const FilePath &executable,
                         const FilePath &qchFile,
                         bool autoRun,
                         bool autoCreate);
    void removeCMakeTool(const Utils::Id &id);
    void apply();

    Utils::Id defaultItemId() const;
    void setDefaultItemId(const Utils::Id &id);

    QString uniqueDisplayName(const QString &base) const;
private:
    Utils::Id m_defaultItemId;
    QList<Utils::Id> m_removedItems;
};

class CMakeToolTreeItem : public TreeItem
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::CMakeSettingsPage)

public:
    CMakeToolTreeItem(const CMakeTool *item, bool changed)
        : m_id(item->id())
        , m_name(item->displayName())
        , m_executable(item->filePath())
        , m_qchFile(item->qchFilePath())
        , m_isAutoRun(item->isAutoRun())
        , m_autoCreateBuildDirectory(item->autoCreateBuildDirectory())
        , m_autodetected(item->isAutoDetected())
        , m_isSupported(item->hasFileApi())
        , m_changed(changed)
    {
        updateErrorFlags();
        m_tooltip = tr("Version: %1<br>Supports fileApi: %2")
                        .arg(QString::fromUtf8(item->version().fullVersion))
                        .arg(item->hasFileApi() ? tr("yes") : tr("no"));
    }

    CMakeToolTreeItem(const QString &name,
                      const FilePath &executable,
                      const FilePath &qchFile,
                      bool autoRun,
                      bool autoCreate,
                      bool autodetected)
        : m_id(Utils::Id::fromString(QUuid::createUuid().toString()))
        , m_name(name)
        , m_executable(executable)
        , m_qchFile(qchFile)
        , m_isAutoRun(autoRun)
        , m_autoCreateBuildDirectory(autoCreate)
        , m_autodetected(autodetected)
    {
        updateErrorFlags();
    }

    void updateErrorFlags()
    {
        const QFileInfo fi = CMakeTool::cmakeExecutable(m_executable).toFileInfo();
        m_pathExists = fi.exists();
        m_pathIsFile = fi.isFile();
        m_pathIsExecutable = fi.isExecutable();
    }

    CMakeToolTreeItem() = default;

    CMakeToolItemModel *model() const { return static_cast<CMakeToolItemModel *>(TreeItem::model()); }

    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DisplayRole: {
            switch (column) {
            case 0: {
                QString name = m_name;
                if (model()->defaultItemId() == m_id)
                    name += tr(" (Default)");
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
            font.setItalic(model()->defaultItemId() == m_id);
            return font;
        }
        case Qt::ToolTipRole: {
            QString result = m_tooltip;
            QString error;
            if (!m_pathExists) {
                error = QCoreApplication::translate(
                    "CMakeProjectManager::Internal::CMakeToolTreeItem",
                    "CMake executable path does not exist.");
            } else if (!m_pathIsFile) {
                error = QCoreApplication::translate(
                    "CMakeProjectManager::Internal::CMakeToolTreeItem",
                    "CMake executable path is not a file.");
            } else if (!m_pathIsExecutable) {
                error = QCoreApplication::translate(
                    "CMakeProjectManager::Internal::CMakeToolTreeItem",
                    "CMake executable path is not executable.");
            } else if (!m_isSupported) {
                error = QCoreApplication::translate(
                    "CMakeProjectManager::Internal::CMakeToolTreeItem",
                    "CMake executable does not provided required IDE integration features.");
            }
            if (result.isEmpty() || error.isEmpty())
                return QString("%1%2").arg(result).arg(error);
            else
                return QString("%1<br><br><b>%2</b>").arg(result).arg(error);
        }
        case Qt::DecorationRole: {
            if (column != 0)
                return QVariant();

            const bool hasError = !m_pathExists || !m_pathIsFile || !m_pathIsExecutable;
            if (hasError)
                return Utils::Icons::CRITICAL.icon();
            return QVariant();
        }
        }
        return QVariant();
    }

    Utils::Id m_id;
    QString m_name;
    QString m_tooltip;
    FilePath m_executable;
    FilePath m_qchFile;
    bool m_isAutoRun = true;
    bool m_pathExists = false;
    bool m_pathIsFile = false;
    bool m_pathIsExecutable = false;
    bool m_autoCreateBuildDirectory = false;
    bool m_autodetected = false;
    bool m_isSupported = false;
    bool m_changed = true;
};

CMakeToolItemModel::CMakeToolItemModel()
{
    setHeader({tr("Name"), tr("Location")});
    rootItem()->appendChild(new StaticTreeItem(tr("Auto-detected")));
    rootItem()->appendChild(new StaticTreeItem(tr("Manual")));

    foreach (const CMakeTool *item, CMakeToolManager::cmakeTools())
        addCMakeTool(item, false);

    CMakeTool *defTool = CMakeToolManager::defaultCMakeTool();
    m_defaultItemId = defTool ? defTool->id() : Utils::Id();
    connect(CMakeToolManager::instance(), &CMakeToolManager::cmakeRemoved,
            this, &CMakeToolItemModel::removeCMakeTool);
    connect(CMakeToolManager::instance(), &CMakeToolManager::cmakeAdded,
            this, [this](const Utils::Id &id) { addCMakeTool(CMakeToolManager::findById(id), false); });

}

QModelIndex CMakeToolItemModel::addCMakeTool(const QString &name,
                                             const FilePath &executable,
                                             const FilePath &qchFile,
                                             const bool autoRun,
                                             const bool autoCreate,
                                             const bool isAutoDetected)
{
    auto item = new CMakeToolTreeItem(name, executable, qchFile, autoRun, autoCreate, isAutoDetected);
    if (isAutoDetected)
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
    if (item->isAutoDetected())
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
    Utils::Id origDefault = origDefTool ? origDefTool->id() : Utils::Id();
    if (origDefault != m_defaultItemId) {
        if (item->m_id == origDefault || item->m_id == m_defaultItemId)
            item->m_changed = true;
    }

    item->update(); // Notify views.
}

void CMakeToolItemModel::updateCMakeTool(const Utils::Id &id,
                                         const QString &displayName,
                                         const FilePath &executable,
                                         const FilePath &qchFile,
                                         bool autoRun,
                                         bool autoCreate)
{
    CMakeToolTreeItem *treeItem = cmakeToolItem(id);
    QTC_ASSERT(treeItem, return );

    treeItem->m_name = displayName;
    treeItem->m_executable = executable;
    treeItem->m_qchFile = qchFile;
    treeItem->m_isAutoRun = autoRun;
    treeItem->m_autoCreateBuildDirectory = autoCreate;

    treeItem->updateErrorFlags();

    reevaluateChangedFlag(treeItem);
}

CMakeToolTreeItem *CMakeToolItemModel::cmakeToolItem(const Utils::Id &id) const
{
    return findItemAtLevel<2>([id](CMakeToolTreeItem *n) { return n->m_id == id; });
}

CMakeToolTreeItem *CMakeToolItemModel::cmakeToolItem(const QModelIndex &index) const
{
    return itemForIndexAtLevel<2>(index);
}

void CMakeToolItemModel::removeCMakeTool(const Utils::Id &id)
{
    if (m_removedItems.contains(id))
        return; // Item has already been removed in the model!

    CMakeToolTreeItem *treeItem = cmakeToolItem(id);
    QTC_ASSERT(treeItem, return);

    destroyItem(treeItem);
    m_removedItems.append(id);
}

void CMakeToolItemModel::apply()
{
    foreach (const Utils::Id &id, m_removedItems)
        CMakeToolManager::deregisterCMakeTool(id);

    QList<CMakeToolTreeItem *> toRegister;
    forItemsAtLevel<2>([&toRegister](CMakeToolTreeItem *item) {
        item->m_changed = false;
        if (CMakeTool *cmake = CMakeToolManager::findById(item->m_id)) {
            cmake->setDisplayName(item->m_name);
            cmake->setFilePath(item->m_executable);
            cmake->setQchFilePath(item->m_qchFile);
            cmake->setAutorun(item->m_isAutoRun);
            cmake->setAutoCreateBuildDirectory(item->m_autoCreateBuildDirectory);
        } else {
            toRegister.append(item);
        }
    });

    foreach (CMakeToolTreeItem *item, toRegister) {
        CMakeTool::Detection detection = item->m_autodetected ? CMakeTool::AutoDetection
                                                              : CMakeTool::ManualDetection;
        auto cmake = std::make_unique<CMakeTool>(detection, item->m_id);
        cmake->setDisplayName(item->m_name);
        cmake->setFilePath(item->m_executable);
        cmake->setQchFilePath(item->m_qchFile);
        if (!CMakeToolManager::registerCMakeTool(std::move(cmake)))
            item->m_changed = true;
    }

    CMakeToolManager::setDefaultCMakeTool(defaultItemId());
}

Utils::Id CMakeToolItemModel::defaultItemId() const
{
    return m_defaultItemId;
}

void CMakeToolItemModel::setDefaultItemId(const Utils::Id &id)
{
    if (m_defaultItemId == id)
        return;

    Utils::Id oldDefaultId = m_defaultItemId;
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

// -----------------------------------------------------------------------
// CMakeToolItemConfigWidget
// -----------------------------------------------------------------------

class CMakeToolItemConfigWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::CMakeSettingsPage)

public:
    explicit CMakeToolItemConfigWidget(CMakeToolItemModel *model);
    void load(const CMakeToolTreeItem *item);
    void store() const;

private:
    void updateQchFilePath();

    CMakeToolItemModel *m_model;
    QLineEdit *m_displayNameLineEdit;
    QCheckBox *m_autoRunCheckBox;
    QCheckBox *m_autoCreateBuildDirectoryCheckBox;
    PathChooser *m_binaryChooser;
    PathChooser *m_qchFileChooser;
    Utils::Id m_id;
    bool m_loadingItem;
};

CMakeToolItemConfigWidget::CMakeToolItemConfigWidget(CMakeToolItemModel *model)
    : m_model(model), m_loadingItem(false)
{
    m_displayNameLineEdit = new QLineEdit(this);

    m_binaryChooser = new PathChooser(this);
    m_binaryChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_binaryChooser->setMinimumWidth(400);
    m_binaryChooser->setHistoryCompleter(QLatin1String("Cmake.Command.History"));
    m_binaryChooser->setCommandVersionArguments({"--version"});

    m_qchFileChooser = new PathChooser(this);
    m_qchFileChooser->setExpectedKind(PathChooser::File);
    m_qchFileChooser->setMinimumWidth(400);
    m_qchFileChooser->setHistoryCompleter(QLatin1String("Cmake.qchFile.History"));
    m_qchFileChooser->setPromptDialogFilter("*.qch");
    m_qchFileChooser->setPromptDialogTitle(tr("CMake .qch File"));

    m_autoRunCheckBox = new QCheckBox;
    m_autoRunCheckBox->setText(tr("Autorun CMake"));
    m_autoRunCheckBox->setToolTip(tr("Automatically run CMake after changes to CMake project files."));

    m_autoCreateBuildDirectoryCheckBox = new QCheckBox;
    m_autoCreateBuildDirectoryCheckBox->setText(tr("Auto-create build directories"));
    m_autoCreateBuildDirectoryCheckBox->setToolTip(tr("Automatically create build directories for CMake projects."));

    auto formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(new QLabel(tr("Name:")), m_displayNameLineEdit);
    formLayout->addRow(new QLabel(tr("Path:")), m_binaryChooser);
    formLayout->addRow(new QLabel(tr("Help file:")), m_qchFileChooser);
    formLayout->addRow(m_autoRunCheckBox);
    formLayout->addRow(m_autoCreateBuildDirectoryCheckBox);

    connect(m_binaryChooser, &PathChooser::rawPathChanged, this, [this]() {
        updateQchFilePath();
        m_qchFileChooser->setBaseDirectory(m_binaryChooser->filePath().parentDir());
        store();
    });
    connect(m_qchFileChooser, &PathChooser::rawPathChanged, this, &CMakeToolItemConfigWidget::store);
    connect(m_displayNameLineEdit, &QLineEdit::textChanged, this, &CMakeToolItemConfigWidget::store);
    connect(m_autoRunCheckBox, &QCheckBox::toggled,
            this, &CMakeToolItemConfigWidget::store);
    connect(m_autoCreateBuildDirectoryCheckBox, &QCheckBox::toggled,
            this, &CMakeToolItemConfigWidget::store);
}

void CMakeToolItemConfigWidget::store() const
{
    if (!m_loadingItem && m_id.isValid())
        m_model->updateCMakeTool(m_id,
                                 m_displayNameLineEdit->text(),
                                 m_binaryChooser->filePath(),
                                 m_qchFileChooser->filePath(),
                                 m_autoRunCheckBox->checkState() == Qt::Checked,
                                 m_autoCreateBuildDirectoryCheckBox->checkState() == Qt::Checked);
}

void CMakeToolItemConfigWidget::updateQchFilePath()
{
    if (m_qchFileChooser->filePath().isEmpty())
        m_qchFileChooser->setFilePath(CMakeTool::searchQchFile(m_binaryChooser->filePath()));
}

void CMakeToolItemConfigWidget::load(const CMakeToolTreeItem *item)
{
    m_loadingItem = true; // avoid intermediate signal handling
    m_id = Utils::Id();
    if (!item) {
        m_loadingItem = false;
        return;
    }

    // Set values:
    m_displayNameLineEdit->setEnabled(!item->m_autodetected);
    m_displayNameLineEdit->setText(item->m_name);

    m_binaryChooser->setReadOnly(item->m_autodetected);
    m_binaryChooser->setFilePath(item->m_executable);

    m_qchFileChooser->setReadOnly(item->m_autodetected);
    m_qchFileChooser->setBaseDirectory(item->m_executable.parentDir());
    m_qchFileChooser->setFilePath(item->m_qchFile);

    m_autoRunCheckBox->setChecked(item->m_isAutoRun);
    m_autoCreateBuildDirectoryCheckBox->setChecked(item->m_autoCreateBuildDirectory);

    m_id = item->m_id;
    m_loadingItem = false;
}

// --------------------------------------------------------------------------
// CMakeToolConfigWidget
// --------------------------------------------------------------------------

class CMakeToolConfigWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::Internal::CMakeToolConfigWidget)

public:
    CMakeToolConfigWidget()
    {
        m_addButton = new QPushButton(tr("Add"), this);

        m_cloneButton = new QPushButton(tr("Clone"), this);
        m_cloneButton->setEnabled(false);

        m_delButton = new QPushButton(tr("Remove"), this);
        m_delButton->setEnabled(false);

        m_makeDefButton = new QPushButton(tr("Make Default"), this);
        m_makeDefButton->setEnabled(false);
        m_makeDefButton->setToolTip(tr("Set as the default CMake Tool to use when creating a new kit or when no value is set."));

        m_container = new DetailsWidget(this);
        m_container->setState(DetailsWidget::NoSummary);
        m_container->setVisible(false);

        m_cmakeToolsView = new QTreeView(this);
        m_cmakeToolsView->setModel(&m_model);
        m_cmakeToolsView->setUniformRowHeights(true);
        m_cmakeToolsView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_cmakeToolsView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_cmakeToolsView->expandAll();

        QHeaderView *header = m_cmakeToolsView->header();
        header->setStretchLastSection(false);
        header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(1, QHeaderView::Stretch);

        auto buttonLayout = new QVBoxLayout();
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->addWidget(m_addButton);
        buttonLayout->addWidget(m_cloneButton);
        buttonLayout->addWidget(m_delButton);
        buttonLayout->addWidget(m_makeDefButton);
        buttonLayout->addItem(new QSpacerItem(10, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

        auto verticalLayout = new QVBoxLayout();
        verticalLayout->addWidget(m_cmakeToolsView);
        verticalLayout->addWidget(m_container);

        auto horizontalLayout = new QHBoxLayout(this);
        horizontalLayout->addLayout(verticalLayout);
        horizontalLayout->addLayout(buttonLayout);

        connect(m_cmakeToolsView->selectionModel(), &QItemSelectionModel::currentChanged,
                this, &CMakeToolConfigWidget::currentCMakeToolChanged, Qt::QueuedConnection);

        connect(m_addButton, &QAbstractButton::clicked,
                this, &CMakeToolConfigWidget::addCMakeTool);
        connect(m_cloneButton, &QAbstractButton::clicked,
                this, &CMakeToolConfigWidget::cloneCMakeTool);
        connect(m_delButton, &QAbstractButton::clicked,
                this, &CMakeToolConfigWidget::removeCMakeTool);
        connect(m_makeDefButton, &QAbstractButton::clicked,
                this, &CMakeToolConfigWidget::setDefaultCMakeTool);

        m_itemConfigWidget = new CMakeToolItemConfigWidget(&m_model);
        m_container->setWidget(m_itemConfigWidget);
    }

    void apply() final;

    void cloneCMakeTool();
    void addCMakeTool();
    void removeCMakeTool();
    void setDefaultCMakeTool();
    void currentCMakeToolChanged(const QModelIndex &newCurrent);

    CMakeToolItemModel m_model;
    QTreeView *m_cmakeToolsView;
    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_delButton;
    QPushButton *m_makeDefButton;
    DetailsWidget *m_container;
    CMakeToolItemConfigWidget *m_itemConfigWidget;
    CMakeToolTreeItem *m_currentItem = nullptr;
};

void CMakeToolConfigWidget::apply()
{
    m_itemConfigWidget->store();
    m_model.apply();
}

void CMakeToolConfigWidget::cloneCMakeTool()
{
    if (!m_currentItem)
        return;

    QModelIndex newItem = m_model.addCMakeTool(tr("Clone of %1").arg(m_currentItem->m_name),
                                               m_currentItem->m_executable,
                                               m_currentItem->m_qchFile,
                                               m_currentItem->m_isAutoRun,
                                               m_currentItem->m_autoCreateBuildDirectory,
                                               false);

    m_cmakeToolsView->setCurrentIndex(newItem);
}

void CMakeToolConfigWidget::addCMakeTool()
{
    QModelIndex newItem = m_model.addCMakeTool(m_model.uniqueDisplayName(tr("New CMake")),
                                               FilePath(),
                                               FilePath(),
                                               true,
                                               false,
                                               false);

    m_cmakeToolsView->setCurrentIndex(newItem);
}

void CMakeToolConfigWidget::removeCMakeTool()
{
    bool delDef = m_model.defaultItemId() == m_currentItem->m_id;
    m_model.removeCMakeTool(m_currentItem->m_id);
    m_currentItem = nullptr;

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
        m_cmakeToolsView->setCurrentIndex(newCurrent->index());
}

void CMakeToolConfigWidget::setDefaultCMakeTool()
{
    if (!m_currentItem)
        return;

    m_model.setDefaultItemId(m_currentItem->m_id);
    m_makeDefButton->setEnabled(false);
}

void CMakeToolConfigWidget::currentCMakeToolChanged(const QModelIndex &newCurrent)
{
    m_currentItem = m_model.cmakeToolItem(newCurrent);
    m_itemConfigWidget->load(m_currentItem);
    m_container->setVisible(m_currentItem);
    m_cloneButton->setEnabled(m_currentItem);
    m_delButton->setEnabled(m_currentItem && !m_currentItem->m_autodetected);
    m_makeDefButton->setEnabled(m_currentItem && (!m_model.defaultItemId().isValid() || m_currentItem->m_id != m_model.defaultItemId()));
}

/////
// CMakeSettingsPage
////

CMakeSettingsPage::CMakeSettingsPage()
{
    setId(Constants::CMAKE_SETTINGS_PAGE_ID);
    setDisplayName(CMakeToolConfigWidget::tr("CMake"));
    setCategory(ProjectExplorer::Constants::KITS_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new CMakeToolConfigWidget; });
}

} // namespace Internal
} // namespace CMakeProjectManager
