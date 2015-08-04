/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "cmakeprojectconstants.h"
#include "cmakesettingspage.h"
#include "cmaketoolmanager.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <coreplugin/icore.h>
#include <utils/environment.h>
#include <utils/detailswidget.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>

#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeView>
#include <QWidget>
#include <QUuid>
#include <QCheckBox>

using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

class CMakeToolTreeItem;

// --------------------------------------------------------------------------
// CMakeToolItemModel
// --------------------------------------------------------------------------

class CMakeToolItemModel : public TreeModel
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::CMakeSettingsPage)

public:
    CMakeToolItemModel();

    CMakeToolTreeItem *cmakeToolItem(const Core::Id &id) const;
    CMakeToolTreeItem *cmakeToolItem(const QModelIndex &index) const;
    QModelIndex addCMakeTool(const QString &name, const FileName &executable, const bool isAutoDetected);
    QModelIndex addCMakeTool(const CMakeTool *item, bool changed);
    TreeItem *autoGroupItem() const;
    TreeItem *manualGroupItem() const;
    void reevaluateChangedFlag(CMakeToolTreeItem *item) const;
    void updateCMakeTool(const Core::Id &id, const QString &displayName, const FileName &executable);
    void removeCMakeTool(const Core::Id &id);
    void apply();

    Core::Id defaultItemId() const;
    void setDefaultItemId(const Core::Id &id);

    QString uniqueDisplayName(const QString &base) const;
private:
    Core::Id m_defaultItemId;
    QList<Core::Id> m_removedItems;
};

class CMakeToolTreeItem : public TreeItem
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::CMakeSettingsPage)

public:
    CMakeToolTreeItem(const CMakeTool *item, bool changed) :
        m_id(item->id()),
        m_name(item->displayName()),
        m_executable(item->cmakeExecutable()),
        m_autodetected(item->isAutoDetected()),
        m_changed(changed)
    {}

    CMakeToolTreeItem(const QString &name, const Utils::FileName &executable, bool autodetected) :
        m_id(Core::Id::fromString(QUuid::createUuid().toString())),
        m_name(name),
        m_executable(executable),
        m_autodetected(autodetected),
        m_changed(true)
    {}

    CMakeToolTreeItem() : m_autodetected(false), m_changed(true) {}

    CMakeToolItemModel *model() const { return static_cast<CMakeToolItemModel *>(TreeItem::model()); }

    QVariant data(int column, int role) const
    {
        switch (role) {
        case Qt::DisplayRole:
            switch (column) {
            case 0: {
                QString name = m_name;
                if (model()->defaultItemId() == m_id)
                    name += tr(" (Default)");
                return name;
            }
            case 1: return m_executable.toUserOutput();
            }

        case Qt::FontRole: {
            QFont font;
            font.setBold(m_changed);
            font.setItalic(model()->defaultItemId() == m_id);
            return font;
        }
        }
        return QVariant();
    }

    Core::Id m_id;
    QString  m_name;
    FileName m_executable;
    bool m_autodetected;
    bool m_changed;
};

CMakeToolItemModel::CMakeToolItemModel()
{
    setHeader(QStringList() << tr("Name") << tr("Location"));
    rootItem()->appendChild(new TreeItem(QStringList() << tr("Auto-detected") << QString() << QString()));
    rootItem()->appendChild(new TreeItem(QStringList() << tr("Manual") << QString() << QString()));

    foreach (const CMakeTool *item, CMakeToolManager::cmakeTools())
        addCMakeTool(item, false);

    CMakeTool *defTool = CMakeToolManager::defaultCMakeTool();
    m_defaultItemId = defTool ? defTool->id() : Core::Id();
}

QModelIndex CMakeToolItemModel::addCMakeTool(const QString &name, const FileName &executable, const bool isAutoDetected)
{
    CMakeToolTreeItem *item = new CMakeToolTreeItem(name, executable, isAutoDetected);
    if (isAutoDetected)
        autoGroupItem()->appendChild(item);
    else
        manualGroupItem()->appendChild(item);

    return item->index();
}

QModelIndex CMakeToolItemModel::addCMakeTool(const CMakeTool *item, bool changed)
{
    CMakeToolTreeItem *treeItem = new CMakeToolTreeItem(item, changed);
    if (item->isAutoDetected())
        autoGroupItem()->appendChild(treeItem);
    else
        manualGroupItem()->appendChild(treeItem);

    return treeItem->index();
}

TreeItem *CMakeToolItemModel::autoGroupItem() const
{
    return rootItem()->child(0);
}

TreeItem *CMakeToolItemModel::manualGroupItem() const
{
    return rootItem()->child(1);
}

void CMakeToolItemModel::reevaluateChangedFlag(CMakeToolTreeItem *item) const
{
    CMakeTool *orig = CMakeToolManager::findById(item->m_id);
    item->m_changed = !orig || orig->displayName() != item->m_name
            || orig->cmakeExecutable() != item->m_executable;

    //make sure the item is marked as changed when the default cmake was changed
    CMakeTool *origDefTool = CMakeToolManager::defaultCMakeTool();
    Core::Id origDefault = origDefTool ? origDefTool->id() : Core::Id();
    if (origDefault != m_defaultItemId) {
        if (item->m_id == origDefault || item->m_id == m_defaultItemId)
            item->m_changed = true;
    }

    item->update(); // Notify views.
}

void CMakeToolItemModel::updateCMakeTool(const Core::Id &id, const QString &displayName,
                                         const FileName &executable)
{
    CMakeToolTreeItem *treeItem = cmakeToolItem(id);
    QTC_ASSERT(treeItem, return);

    treeItem->m_name = displayName;
    treeItem->m_executable = executable;

    reevaluateChangedFlag(treeItem);
}

CMakeToolTreeItem *CMakeToolItemModel::cmakeToolItem(const Core::Id &id) const
{
    auto matcher = [id](CMakeToolTreeItem *n) { return n->m_id == id; };
    return findItemAtLevel<CMakeToolTreeItem *>(2, matcher);
}

CMakeToolTreeItem *CMakeToolItemModel::cmakeToolItem(const QModelIndex &index) const
{
    TreeItem *item = itemForIndex(index);
    if (item->level() == 2)
        return static_cast<CMakeToolTreeItem *>(item);
    return 0;
}

void CMakeToolItemModel::removeCMakeTool(const Core::Id &id)
{
    CMakeToolTreeItem *treeItem = cmakeToolItem(id);
    QTC_ASSERT(treeItem, return);

    delete takeItem(treeItem);
    m_removedItems.append(id);
}

void CMakeToolItemModel::apply()
{
    foreach (const Core::Id &id, m_removedItems)
        CMakeToolManager::deregisterCMakeTool(id);

    foreach (auto item, itemsAtLevel<CMakeToolTreeItem *>(2)) {
        item->m_changed = false;

        bool isNew = false;
        CMakeTool *cmake = CMakeToolManager::findById(item->m_id);
        if (!cmake) {
            isNew = true;
            CMakeTool::Detection detection = item->m_autodetected ? CMakeTool::AutoDetection
                                                                  : CMakeTool::ManualDetection;
            cmake = new CMakeTool(detection, item->m_id);
        }

        cmake->setDisplayName(item->m_name);
        cmake->setCMakeExecutable(item->m_executable);

        if (isNew) {
            if (!CMakeToolManager::registerCMakeTool(cmake)) {
                delete cmake;
                item->m_changed = true;
            }
        }
    }

    CMakeToolManager::setDefaultCMakeTool(defaultItemId());
}

Core::Id CMakeToolItemModel::defaultItemId() const
{
    return m_defaultItemId;
}

void CMakeToolItemModel::setDefaultItemId(const Core::Id &id)
{
    if (m_defaultItemId == id)
        return;

    Core::Id oldDefaultId = m_defaultItemId;
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
    foreach (CMakeToolTreeItem *item, itemsAtLevel<CMakeToolTreeItem *>(2))
        names << item->m_name;

    return ProjectExplorer::Project::makeUnique(base, names);
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
    CMakeToolItemModel *m_model;
    QLineEdit *m_displayNameLineEdit;
    PathChooser *m_binaryChooser;
    Core::Id m_id;
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

    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(new QLabel(tr("Name:")), m_displayNameLineEdit);
    formLayout->addRow(new QLabel(tr("Path:")), m_binaryChooser);

    connect(m_binaryChooser, &PathChooser::changed,
            this, &CMakeToolItemConfigWidget::store);
    connect(m_displayNameLineEdit, &QLineEdit::textChanged,
            this, &CMakeToolItemConfigWidget::store);
}

void CMakeToolItemConfigWidget::store() const
{
    if (!m_loadingItem && m_id.isValid())
        m_model->updateCMakeTool(m_id, m_displayNameLineEdit->text(), m_binaryChooser->fileName());
}

void CMakeToolItemConfigWidget::load(const CMakeToolTreeItem *item)
{
    m_loadingItem = true; // avoid intermediate signal handling
    m_id = Core::Id();
    if (!item) {
        m_loadingItem = false;
        return;
    }

    // Set values:
    m_displayNameLineEdit->setEnabled(!item->m_autodetected);
    m_displayNameLineEdit->setText(item->m_name);

    m_binaryChooser->setReadOnly(item->m_autodetected);
    m_binaryChooser->setFileName(item->m_executable);

    m_id = item->m_id;
    m_loadingItem = false;
}

// --------------------------------------------------------------------------
// CMakeToolConfigWidget
// --------------------------------------------------------------------------

class CMakeToolConfigWidget : public QWidget
{
    Q_OBJECT
public:
    CMakeToolConfigWidget() : m_currentItem(0)
    {
        m_addButton = new QPushButton(tr("Add"), this);

        m_cloneButton = new QPushButton(tr("Clone"), this);
        m_cloneButton->setEnabled(false);

        m_delButton = new QPushButton(tr("Remove"), this);
        m_delButton->setEnabled(false);

        m_makeDefButton = new QPushButton(tr("Make Default"), this);
        m_makeDefButton->setEnabled(false);
        m_makeDefButton->setToolTip(tr("Set as the default CMake Tool to use when creating a new Kit, or no value is set."));

        m_preferNinjaCheckBox = new QCheckBox(tr("Prefer Ninja generator (CMake 2.8.9 or higher required)"));
        m_preferNinjaCheckBox->setChecked(CMakeToolManager::preferNinja());

        m_container = new DetailsWidget(this);
        m_container->setState(DetailsWidget::NoSummary);
        m_container->setVisible(false);

        m_cmakeToolsView = new QTreeView(this);
        m_cmakeToolsView->setModel(&m_model);
        m_cmakeToolsView->setUniformRowHeights(true);
        m_cmakeToolsView->setRootIsDecorated(false);
        m_cmakeToolsView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_cmakeToolsView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_cmakeToolsView->expandAll();

        QHeaderView *header = m_cmakeToolsView->header();
        header->setStretchLastSection(false);
        header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(1, QHeaderView::Stretch);

        QVBoxLayout *buttonLayout = new QVBoxLayout();
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->addWidget(m_addButton);
        buttonLayout->addWidget(m_cloneButton);
        buttonLayout->addWidget(m_delButton);
        buttonLayout->addWidget(m_makeDefButton);
        buttonLayout->addItem(new QSpacerItem(10, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

        QVBoxLayout *verticalLayout = new QVBoxLayout();
        verticalLayout->addWidget(m_cmakeToolsView);
        verticalLayout->addWidget(m_container);
        verticalLayout->addWidget(m_preferNinjaCheckBox);

        QHBoxLayout *horizontalLayout = new QHBoxLayout(this);
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

    void apply();
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
    QCheckBox *m_preferNinjaCheckBox;
    DetailsWidget *m_container;
    CMakeToolItemConfigWidget *m_itemConfigWidget;
    CMakeToolTreeItem *m_currentItem;
};

void CMakeToolConfigWidget::apply()
{
    m_model.apply();
    CMakeToolManager::setPreferNinja(m_preferNinjaCheckBox->checkState() == Qt::Checked);
}

void CMakeToolConfigWidget::cloneCMakeTool()
{
    if (!m_currentItem)
        return;

    QModelIndex newItem = m_model.addCMakeTool(tr("Clone of %1").arg(m_currentItem->m_name),
                                               m_currentItem->m_executable,
                                               false);

    m_cmakeToolsView->setCurrentIndex(newItem);
}

void CMakeToolConfigWidget::addCMakeTool()
{
    QModelIndex newItem = m_model.addCMakeTool(m_model.uniqueDisplayName(tr("New CMake")),
                                               FileName(), false);

    m_cmakeToolsView->setCurrentIndex(newItem);
}

void CMakeToolConfigWidget::removeCMakeTool()
{
    bool delDef = m_model.defaultItemId() == m_currentItem->m_id;
    m_model.removeCMakeTool(m_currentItem->m_id);
    m_currentItem = 0;

    if (delDef) {
        CMakeToolTreeItem *it = static_cast<CMakeToolTreeItem *>(m_model.autoGroupItem()->firstChild());
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

CMakeSettingsPage::CMakeSettingsPage() : m_widget(0)
{
    setId(Constants::CMAKE_SETTINGSPAGE_ID);
    setDisplayName(tr("CMake"));
    setCategory(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
       ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
}

CMakeSettingsPage::~CMakeSettingsPage()
{
}

QWidget *CMakeSettingsPage::widget()
{
    if (!m_widget)
        m_widget = new CMakeToolConfigWidget;
    return m_widget;
}

void CMakeSettingsPage::apply()
{
    QTC_ASSERT(m_widget, return);
    m_widget->m_itemConfigWidget->store();
    m_widget->m_model.apply();
}

void CMakeSettingsPage::finish()
{
    delete m_widget;
    m_widget = 0;
}

} // namespace Internal
} // namespace CMakeProjectManager

#include "cmakesettingspage.moc"
