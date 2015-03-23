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

#include "debuggeroptionspage.h"
#include "debuggeritemmanager.h"
#include "debuggeritem.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/detailswidget.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>
#include <utils/winutils.h>

#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QTreeView>
#include <QWidget>

using namespace Utils;

namespace Debugger {
namespace Internal {

const char debuggingToolsWikiLinkC[] = "http://wiki.qt.io/Qt_Creator_Windows_Debugging";

// --------------------------------------------------------------------------
// DebuggerTreeItem
// --------------------------------------------------------------------------

class DebuggerTreeItem : public TreeItem
{
public:
    DebuggerTreeItem(const DebuggerItem &item, bool changed) : m_item(item), m_changed(changed) {}

    QVariant data(int column, int role) const
    {
        switch (role) {
            case Qt::DisplayRole:
                switch (column) {
                case 0: return m_item.displayName();
                case 1: return m_item.command().toUserOutput();
                case 2: return m_item.engineTypeName();
                }

            case Qt::FontRole: {
                QFont font;
                font.setBold(m_changed);
                return font;
            }
        }
        return QVariant();
    }

    DebuggerItem m_item;
    bool m_changed;
};

// --------------------------------------------------------------------------
// DebuggerItemModel
// --------------------------------------------------------------------------

class DebuggerItemModel : public TreeModel
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::DebuggerOptionsPage)

public:
    DebuggerItemModel();

    QModelIndex lastIndex() const;
    void setCurrentIndex(const QModelIndex &index);
    DebuggerItem *currentDebugger() const;
    void addDebugger(const DebuggerItem &item, bool changed);
    void updateDebugger(const DebuggerItem &item);
    void removeCurrentDebugger();
    void apply();

private:
    DebuggerTreeItem *m_currentTreeItem;
    QStringList removed;

    QList<QVariant> m_removedItems;
};

DebuggerItemModel::DebuggerItemModel()
    : m_currentTreeItem(0)
{
    setHeader(QStringList() << tr("Name") << tr("Location") << tr("Type"));
    rootItem()->appendChild(new TreeItem(QStringList() << tr("Auto-detected") << QString() << QString()));
    rootItem()->appendChild(new TreeItem(QStringList() << tr("Manual") << QString() << QString()));

    foreach (const DebuggerItem &item, DebuggerItemManager::debuggers())
        addDebugger(item, false);
}

void DebuggerItemModel::addDebugger(const DebuggerItem &item, bool changed)
{
    int group = item.isAutoDetected() ? 0 : 1;
    rootItem()->child(group)->appendChild(new DebuggerTreeItem(item, changed));
}

void DebuggerItemModel::updateDebugger(const DebuggerItem &item)
{
    auto matcher = [item](DebuggerTreeItem *n) { return n->m_item.m_id == item.id(); };
    DebuggerTreeItem *treeItem = findItemAtLevel<DebuggerTreeItem *>(2, matcher);
    QTC_ASSERT(treeItem, return);

    TreeItem *parent = treeItem->parent();
    QTC_ASSERT(parent, return);

    const DebuggerItem *orig = DebuggerItemManager::findById(item.id());
    treeItem->m_changed = !orig || *orig != item;
    treeItem->m_item = item;
    treeItem->update(); // Notify views.
}

QModelIndex DebuggerItemModel::lastIndex() const
{
    TreeItem *manualGroup = rootItem()->lastChild();
    TreeItem *lastItem = manualGroup->lastChild();
    return lastItem ? indexFromItem(lastItem) : QModelIndex();
}

DebuggerItem *DebuggerItemModel::currentDebugger() const
{
    return m_currentTreeItem ? &m_currentTreeItem->m_item : 0;
}

void DebuggerItemModel::removeCurrentDebugger()
{
    QTC_ASSERT(m_currentTreeItem, return);
    QVariant id = m_currentTreeItem->m_item.id();
    DebuggerTreeItem *treeItem = m_currentTreeItem;
    m_currentTreeItem = 0;
    removeItem(treeItem);
    delete treeItem;
    m_removedItems.append(id);
}

void DebuggerItemModel::apply()
{
    foreach (const QVariant &id, m_removedItems)
        DebuggerItemManager::deregisterDebugger(id);

    foreach (auto item, treeLevelItems<DebuggerTreeItem *>(2)) {
        item->m_changed = false;
        DebuggerItemManager::updateOrAddDebugger(item->m_item);
    }
}

void DebuggerItemModel::setCurrentIndex(const QModelIndex &index)
{
    TreeItem *treeItem = itemFromIndex(index);
    m_currentTreeItem = treeItem && treeItem->level() == 2 ? static_cast<DebuggerTreeItem *>(treeItem) : 0;
}

// -----------------------------------------------------------------------
// DebuggerItemConfigWidget
// -----------------------------------------------------------------------

class DebuggerItemConfigWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::DebuggerOptionsPage)

public:
    explicit DebuggerItemConfigWidget(DebuggerItemModel *model);
    void load(const DebuggerItem *item);
    void store() const;

private:
    void binaryPathHasChanged();
    DebuggerItem item() const;
    void setAbis(const QStringList &abiNames);

    DebuggerItemModel *m_model;
    QLineEdit *m_displayNameLineEdit;
    QLineEdit *m_typeLineEdit;
    QLabel *m_cdbLabel;
    QLineEdit *m_versionLabel;
    PathChooser *m_binaryChooser;
    QLineEdit *m_abis;
    bool m_autodetected;
    DebuggerEngineType m_engineType;
    QVariant m_id;
};

DebuggerItemConfigWidget::DebuggerItemConfigWidget(DebuggerItemModel *model)
    : m_model(model)
{
    m_displayNameLineEdit = new QLineEdit(this);

    m_typeLineEdit = new QLineEdit(this);
    m_typeLineEdit->setEnabled(false);

    m_binaryChooser = new PathChooser(this);
    m_binaryChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_binaryChooser->setMinimumWidth(400);
    m_binaryChooser->setHistoryCompleter(QLatin1String("DebuggerPaths"));

    m_cdbLabel = new QLabel(this);
    m_cdbLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_cdbLabel->setOpenExternalLinks(true);

    m_versionLabel = new QLineEdit(this);
    m_versionLabel->setPlaceholderText(tr("Unknown"));
    m_versionLabel->setEnabled(false);

    m_abis = new QLineEdit(this);
    m_abis->setEnabled(false);

    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(new QLabel(tr("Name:")), m_displayNameLineEdit);
    formLayout->addRow(m_cdbLabel);
    formLayout->addRow(new QLabel(tr("Path:")), m_binaryChooser);
    formLayout->addRow(new QLabel(tr("Type:")), m_typeLineEdit);
    formLayout->addRow(new QLabel(tr("ABIs:")), m_abis);
    formLayout->addRow(new QLabel(tr("Version:")), m_versionLabel);

    connect(m_binaryChooser, &PathChooser::changed,
            this, &DebuggerItemConfigWidget::binaryPathHasChanged);
    connect(m_displayNameLineEdit, &QLineEdit::textChanged,
            this, &DebuggerItemConfigWidget::store);
}

DebuggerItem DebuggerItemConfigWidget::item() const
{
    DebuggerItem item(m_id);
    item.setUnexpandedDisplayName(m_displayNameLineEdit->text());
    item.setCommand(m_binaryChooser->fileName());
    item.setAutoDetected(m_autodetected);
    QList<ProjectExplorer::Abi> abiList;
    foreach (const QString &a, m_abis->text().split(QRegExp(QLatin1String("[^A-Za-z0-9-_]+")))) {
        if (a.isNull())
            continue;
        abiList << a;
    }
    item.setAbis(abiList);
    item.setVersion(m_versionLabel->text());
    item.setEngineType(m_engineType);
    return item;
}

void DebuggerItemConfigWidget::store() const
{
    if (!m_id.isNull())
        m_model->updateDebugger(item());
}

void DebuggerItemConfigWidget::setAbis(const QStringList &abiNames)
{
    m_abis->setText(abiNames.join(QLatin1String(", ")));
}

void DebuggerItemConfigWidget::load(const DebuggerItem *item)
{
    m_id = QVariant(); // reset Id to avoid intermediate signal handling
    if (!item)
        return;

    // Set values:
    m_autodetected = item->isAutoDetected();

    m_displayNameLineEdit->setEnabled(!item->isAutoDetected());
    m_displayNameLineEdit->setText(item->unexpandedDisplayName());

    m_typeLineEdit->setText(item->engineTypeName());

    m_binaryChooser->setReadOnly(item->isAutoDetected());
    m_binaryChooser->setFileName(item->command());

    QString text;
    QString versionCommand;
    if (item->engineType() == CdbEngineType) {
        const bool is64bit = is64BitWindowsSystem();
        const QString versionString = is64bit ? tr("64-bit version") : tr("32-bit version");
        //: Label text for path configuration. %2 is "x-bit version".
        text = tr("<html><body><p>Specify the path to the "
                  "<a href=\"%1\">Windows Console Debugger executable</a>"
                  " (%2) here.</p>""</body></html>").
                arg(QLatin1String(debuggingToolsWikiLinkC), versionString);
        versionCommand = QLatin1String("-version");
    } else {
        versionCommand = QLatin1String("--version");
    }

    m_cdbLabel->setText(text);
    m_cdbLabel->setVisible(!text.isEmpty());
    m_binaryChooser->setCommandVersionArguments(QStringList(versionCommand));
    m_versionLabel->setText(item->version());
    setAbis(item->abiNames());
    m_engineType = item->engineType();
    m_id = item->id();
}

void DebuggerItemConfigWidget::binaryPathHasChanged()
{
    // Ignore change if this is no valid DebuggerItem
    if (!m_id.isValid())
        return;

    DebuggerItem tmp;
    QFileInfo fi = QFileInfo(m_binaryChooser->path());
    if (fi.isExecutable()) {
        tmp = item();
        tmp.reinitializeFromFile();
    }

    setAbis(tmp.abiNames());
    m_versionLabel->setText(tmp.version());
    m_engineType = tmp.engineType();
    m_typeLineEdit->setText(tmp.engineTypeName());

    store();
}

// --------------------------------------------------------------------------
// DebuggerConfigWidget
// --------------------------------------------------------------------------

class DebuggerConfigWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::DebuggerOptionsPage)
public:
    DebuggerConfigWidget()
    {
        m_addButton = new QPushButton(tr("Add"), this);

        m_cloneButton = new QPushButton(tr("Clone"), this);
        m_cloneButton->setEnabled(false);

        m_delButton = new QPushButton(tr("Remove"), this);
        m_delButton->setEnabled(false);

        m_container = new DetailsWidget(this);
        m_container->setState(DetailsWidget::NoSummary);
        m_container->setVisible(false);

        m_debuggerView = new QTreeView(this);
        m_debuggerView->setModel(&m_model);
        m_debuggerView->setUniformRowHeights(true);
        m_debuggerView->setRootIsDecorated(false);
        m_debuggerView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_debuggerView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_debuggerView->expandAll();

        auto header = m_debuggerView->header();
        header->setStretchLastSection(false);
        header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(2, QHeaderView::Stretch);

        auto buttonLayout = new QVBoxLayout();
        buttonLayout->setSpacing(6);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->addWidget(m_addButton);
        buttonLayout->addWidget(m_cloneButton);
        buttonLayout->addWidget(m_delButton);
        buttonLayout->addItem(new QSpacerItem(10, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

        auto verticalLayout = new QVBoxLayout();
        verticalLayout->addWidget(m_debuggerView);
        verticalLayout->addWidget(m_container);

        auto horizontalLayout = new QHBoxLayout(this);
        horizontalLayout->addLayout(verticalLayout);
        horizontalLayout->addLayout(buttonLayout);

        connect(m_debuggerView->selectionModel(), &QItemSelectionModel::currentChanged,
                this, &DebuggerConfigWidget::currentDebuggerChanged, Qt::QueuedConnection);

        connect(m_addButton, &QAbstractButton::clicked,
                this, &DebuggerConfigWidget::addDebugger, Qt::QueuedConnection);
        connect(m_cloneButton, &QAbstractButton::clicked,
                this, &DebuggerConfigWidget::cloneDebugger, Qt::QueuedConnection);
        connect(m_delButton, &QAbstractButton::clicked,
                this, &DebuggerConfigWidget::removeDebugger, Qt::QueuedConnection);

        m_itemConfigWidget = new DebuggerItemConfigWidget(&m_model);
        m_container->setWidget(m_itemConfigWidget);
    }

    void cloneDebugger();
    void addDebugger();
    void removeDebugger();
    void currentDebuggerChanged(const QModelIndex &newCurrent);
    void updateState();

    DebuggerItemModel m_model;
    QTreeView *m_debuggerView;
    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_delButton;
    DetailsWidget *m_container;
    DebuggerItemConfigWidget *m_itemConfigWidget;
};

void DebuggerConfigWidget::cloneDebugger()
{
    DebuggerItem *item = m_model.currentDebugger();
    if (!item)
        return;

    DebuggerItem newItem;
    newItem.createId();
    newItem.setCommand(item->command());
    newItem.setUnexpandedDisplayName(DebuggerItemManager::uniqueDisplayName(tr("Clone of %1").arg(item->displayName())));
    newItem.reinitializeFromFile();
    newItem.setAutoDetected(false);
    m_model.addDebugger(newItem, true);
    m_debuggerView->setCurrentIndex(m_model.lastIndex());
}

void DebuggerConfigWidget::addDebugger()
{
    DebuggerItem item;
    item.createId();
    item.setAutoDetected(false);
    item.setEngineType(NoEngineType);
    item.setUnexpandedDisplayName(DebuggerItemManager::uniqueDisplayName(tr("New Debugger")));
    item.setAutoDetected(false);
    m_model.addDebugger(item, true);
    m_debuggerView->setCurrentIndex(m_model.lastIndex());
}

void DebuggerConfigWidget::removeDebugger()
{
    m_model.removeCurrentDebugger();
    m_debuggerView->setCurrentIndex(m_model.lastIndex());
}

void DebuggerConfigWidget::currentDebuggerChanged(const QModelIndex &newCurrent)
{
    m_model.setCurrentIndex(newCurrent);

    DebuggerItem *item = m_model.currentDebugger();

    m_itemConfigWidget->load(item);
    m_container->setVisible(item);
    m_cloneButton->setEnabled(item && item->isValid() && item->canClone());
    m_delButton->setEnabled(item && !item->isAutoDetected());
}

// --------------------------------------------------------------------------
// DebuggerOptionsPage
// --------------------------------------------------------------------------

DebuggerOptionsPage::DebuggerOptionsPage()
{
    setId(ProjectExplorer::Constants::DEBUGGER_SETTINGS_PAGE_ID);
    setDisplayName(tr("Debuggers"));
    setCategory(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
        ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
}

QWidget *DebuggerOptionsPage::widget()
{
    if (!m_configWidget)
        m_configWidget = new DebuggerConfigWidget;
    return m_configWidget;
}

void DebuggerOptionsPage::apply()
{
    QTC_ASSERT(m_configWidget, return);
    m_configWidget->m_itemConfigWidget->store();
    m_configWidget->m_model.apply();
}

void DebuggerOptionsPage::finish()
{
    delete m_configWidget;
    m_configWidget = 0;
}

} // namespace Internal
} // namespace Debugger
