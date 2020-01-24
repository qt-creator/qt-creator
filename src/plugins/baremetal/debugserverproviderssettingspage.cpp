/****************************************************************************
**
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "baremetalconstants.h"

#include "debugserverprovidermanager.h"
#include "debugserverproviderssettingspage.h"
#include "idebugserverprovider.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSpacerItem>
#include <QTextStream>
#include <QTreeView>
#include <QVBoxLayout>

using namespace Debugger;
using namespace Utils;

namespace BareMetal {
namespace Internal {

// DebugServerProviderNode

enum {
    ProviderNameColumn = 0,
    ProviderTypeColumn,
    ProviderEngineColumn
};

static QString engineTypeName(DebuggerEngineType engineType)
{
    switch (engineType) {
    case NoEngineType:
        return DebugServerProviderModel::tr("Not recognized");
    case GdbEngineType:
        return DebugServerProviderModel::tr("GDB");
    case UvscEngineType:
        return DebugServerProviderModel::tr("UVSC");
    default:
        return {};
    }
}

static QString engineTypeDescription(DebuggerEngineType engineType)
{
    switch (engineType) {
    case NoEngineType:
        return DebugServerProviderModel::tr("Not recognized");
    case GdbEngineType:
        return DebugServerProviderModel::tr("GDB compatible provider engine\n" \
                                            "(used together with the GDB debuggers).");
    case UvscEngineType:
        return DebugServerProviderModel::tr("UVSC compatible provider engine\n" \
                                            "(used together with the KEIL uVision).");
    default:
        return {};
    }
}

class DebugServerProviderNode final : public TreeItem
{
public:
    explicit DebugServerProviderNode(IDebugServerProvider *provider, bool changed = false)
        : provider(provider), changed(changed)
    {
    }

    QVariant data(int column, int role) const final
    {
        if (role == Qt::FontRole) {
            QFont f = QApplication::font();
            if (changed)
                f.setBold(true);
            return f;
        }

        if (role == Qt::DisplayRole) {
            if (column == ProviderNameColumn)
                return provider->displayName();
            if (column == ProviderTypeColumn)
                return provider->typeDisplayName();
            if (column == ProviderEngineColumn)
                return engineTypeName(provider->engineType());
        } else if (role == Qt::ToolTipRole) {
            if (column == ProviderEngineColumn)
                return engineTypeDescription(provider->engineType());
        }

        return {};
    }

    IDebugServerProvider *provider = nullptr;
    IDebugServerProviderConfigWidget *widget = nullptr;
    bool changed = false;
};

// DebugServerProviderModel

DebugServerProviderModel::DebugServerProviderModel()
{
    setHeader({tr("Name"), tr("Type"), tr("Engine")});

    const DebugServerProviderManager *manager = DebugServerProviderManager::instance();

    connect(manager, &DebugServerProviderManager::providerAdded,
            this, &DebugServerProviderModel::addProvider);
    connect(manager, &DebugServerProviderManager::providerRemoved,
            this, &DebugServerProviderModel::removeProvider);

    for (IDebugServerProvider *p : DebugServerProviderManager::providers())
        addProvider(p);
}

IDebugServerProvider *DebugServerProviderModel::provider(const QModelIndex &index) const
{
    if (const DebugServerProviderNode *node = nodeForIndex(index))
        return node->provider;

    return nullptr;
}

DebugServerProviderNode *DebugServerProviderModel::nodeForIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    return static_cast<DebugServerProviderNode *>(itemForIndex(index));
}

void DebugServerProviderModel::apply()
{
    // Remove unused providers
    for (IDebugServerProvider *provider : qAsConst(m_providersToRemove))
        DebugServerProviderManager::deregisterProvider(provider);
    QTC_ASSERT(m_providersToRemove.isEmpty(), m_providersToRemove.clear());

    // Update providers
    for (TreeItem *item : *rootItem()) {
        const auto n = static_cast<DebugServerProviderNode *>(item);
        if (!n->changed)
            continue;

        QTC_CHECK(n->provider);
        if (n->widget)
            n->widget->apply();

        n->changed = false;
        n->update();
    }

    // Add new (and already updated) providers
    QStringList skippedProviders;
    for (IDebugServerProvider *provider: qAsConst(m_providersToAdd)) {
        if (!DebugServerProviderManager::registerProvider(provider))
            skippedProviders << provider->displayName();
    }

    m_providersToAdd.clear();

    if (!skippedProviders.isEmpty()) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             tr("Duplicate Providers Detected"),
                             tr("The following providers were already configured:<br>"
                                "&nbsp;%1<br>"
                                "They were not configured again.")
                             .arg(skippedProviders.join(",<br>&nbsp;")));
    }
}

DebugServerProviderNode *DebugServerProviderModel::findNode(const IDebugServerProvider *provider) const
{
    auto test = [provider](TreeItem *item) {
        return static_cast<DebugServerProviderNode *>(item)->provider == provider;
    };

    return static_cast<DebugServerProviderNode *>(Utils::findOrDefault(*rootItem(), test));
}

QModelIndex DebugServerProviderModel::indexForProvider(IDebugServerProvider *provider) const
{
    const DebugServerProviderNode *n = findNode(provider);
    return n ? indexForItem(n) : QModelIndex();
}

void DebugServerProviderModel::markForRemoval(IDebugServerProvider *provider)
{
    DebugServerProviderNode *n = findNode(provider);
    QTC_ASSERT(n, return);
    destroyItem(n);

    if (m_providersToAdd.contains(provider)) {
        m_providersToAdd.removeOne(provider);
        delete provider;
    } else {
        m_providersToRemove.append(provider);
    }
}

void DebugServerProviderModel::markForAddition(IDebugServerProvider *provider)
{
    DebugServerProviderNode *n = createNode(provider, true);
    rootItem()->appendChild(n);
    m_providersToAdd.append(provider);
}

DebugServerProviderNode *DebugServerProviderModel::createNode(
        IDebugServerProvider *provider, bool changed)
{
    const auto node = new DebugServerProviderNode(provider, changed);
    node->widget = provider->configurationWidget();
    connect(node->widget, &IDebugServerProviderConfigWidget::dirty, this, [node] {
        node->changed = true;
        node->update();
    });
    return node;
}

void DebugServerProviderModel::addProvider(IDebugServerProvider *provider)
{
    if (findNode(provider))
        m_providersToAdd.removeOne(provider);
    else
        rootItem()->appendChild(createNode(provider, false));

    emit providerStateChanged();
}

void DebugServerProviderModel::removeProvider(IDebugServerProvider *provider)
{
    m_providersToRemove.removeAll(provider);
    if (DebugServerProviderNode *n = findNode(provider))
        destroyItem(n);

    emit providerStateChanged();
}

// DebugServerProvidersSettingsWidget

class DebugServerProvidersSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(BareMetal::Internal::DebugServerProvidersSettingsPage)

public:
    DebugServerProvidersSettingsWidget();

    void apply() final { m_model.apply(); }

    void providerSelectionChanged();
    void removeProvider();
    void updateState();

private:
    void addProviderToModel(IDebugServerProvider *p);
    QModelIndex currentIndex() const;

    DebugServerProviderModel m_model;
    QItemSelectionModel *m_selectionModel = nullptr;
    QTreeView *m_providerView = nullptr;
    Utils::DetailsWidget *m_container = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_cloneButton = nullptr;
    QPushButton *m_delButton = nullptr;
};

DebugServerProvidersSettingsWidget::DebugServerProvidersSettingsWidget()
{
    m_providerView = new QTreeView(this);
    m_providerView->setUniformRowHeights(true);
    m_providerView->header()->setStretchLastSection(false);

    m_addButton = new QPushButton(tr("Add"), this);
    m_cloneButton = new QPushButton(tr("Clone"), this);
    m_delButton = new QPushButton(tr("Remove"), this);

    m_container = new Utils::DetailsWidget(this);
    m_container->setState(Utils::DetailsWidget::NoSummary);
    m_container->setMinimumWidth(500);
    m_container->setVisible(false);

    const auto buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(6);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_cloneButton);
    buttonLayout->addWidget(m_delButton);
    const auto spacerItem = new QSpacerItem(40, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);
    buttonLayout->addItem(spacerItem);

    const auto verticalLayout = new QVBoxLayout;
    verticalLayout->addWidget(m_providerView);
    verticalLayout->addLayout(buttonLayout);

    const auto horizontalLayout = new QHBoxLayout;
    horizontalLayout->addLayout(verticalLayout);
    horizontalLayout->addWidget(m_container);

    const auto groupBox = new QGroupBox(tr("Debug Server Providers"), this);
    groupBox->setLayout(horizontalLayout);

    const auto topLayout = new QVBoxLayout(this);
    topLayout->addWidget(groupBox);

    connect(&m_model, &DebugServerProviderModel::providerStateChanged,
            this, &DebugServerProvidersSettingsWidget::updateState);

    m_providerView->setModel(&m_model);

    const auto headerView = m_providerView->header();
    headerView->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    headerView->setSectionResizeMode(1, QHeaderView::Stretch);
    m_providerView->expandAll();

    m_selectionModel = m_providerView->selectionModel();

    connect(m_selectionModel, &QItemSelectionModel::selectionChanged,
            this, &DebugServerProvidersSettingsWidget::providerSelectionChanged);

    connect(DebugServerProviderManager::instance(), &DebugServerProviderManager::providersChanged,
            this, &DebugServerProvidersSettingsWidget::providerSelectionChanged);

    // Set up add menu:
    const auto addMenu = new QMenu(m_addButton);

    for (const auto f : DebugServerProviderManager::factories()) {
        const auto action = new QAction(addMenu);
        action->setText(f->displayName());
        connect(action, &QAction::triggered, this, [this, f] { addProviderToModel(f->create()); });
        addMenu->addAction(action);
    }

    connect(m_cloneButton, &QAbstractButton::clicked, this, [this] {
        if (const IDebugServerProvider *old = m_model.provider(currentIndex())) {
            QString id = old->id();
            for (const auto f : DebugServerProviderManager::factories()) {
                if (id.startsWith(f->id())) {
                    IDebugServerProvider *p = f->create();
                    p->fromMap(old->toMap());
                    p->setDisplayName(tr("Clone of %1").arg(old->displayName()));
                    p->resetId();
                    addProviderToModel(p);
                }
            }
        }
    });

    m_addButton->setMenu(addMenu);

    connect(m_delButton, &QPushButton::clicked,
            this, &DebugServerProvidersSettingsWidget::removeProvider);

    updateState();
}

void DebugServerProvidersSettingsWidget::providerSelectionChanged()
{
    if (!m_container)
        return;
    const QModelIndex current = currentIndex();
    QWidget *w = m_container->takeWidget(); // Prevent deletion.
    if (w)
        w->setVisible(false);

    const DebugServerProviderNode *node = m_model.nodeForIndex(current);
    w = node ? node->widget : nullptr;
    m_container->setWidget(w);
    m_container->setVisible(w != nullptr);
    updateState();
}

void DebugServerProvidersSettingsWidget::addProviderToModel(IDebugServerProvider *provider)
{
    QTC_ASSERT(provider, return);
    m_model.markForAddition(provider);

    m_selectionModel->select(m_model.indexForProvider(provider),
                             QItemSelectionModel::Clear
                             | QItemSelectionModel::SelectCurrent
                             | QItemSelectionModel::Rows);
}

void DebugServerProvidersSettingsWidget::removeProvider()
{
    if (IDebugServerProvider *p = m_model.provider(currentIndex()))
        m_model.markForRemoval(p);
}

void DebugServerProvidersSettingsWidget::updateState()
{
    if (!m_cloneButton)
        return;

    bool canCopy = false;
    bool canDelete = false;
    if (const IDebugServerProvider *p = m_model.provider(currentIndex())) {
        canCopy = p->isValid();
        canDelete = true;
    }

    m_cloneButton->setEnabled(canCopy);
    m_delButton->setEnabled(canDelete);
}

QModelIndex DebugServerProvidersSettingsWidget::currentIndex() const
{
    if (!m_selectionModel)
        return {};

    const QModelIndexList rows = m_selectionModel->selectedRows();
    if (rows.count() != 1)
        return {};
    return rows.at(0);
}

// DebugServerProvidersSettingsPage

DebugServerProvidersSettingsPage::DebugServerProvidersSettingsPage()
{
    setId(Constants::DEBUG_SERVER_PROVIDERS_SETTINGS_ID);
    setDisplayName(DebugServerProvidersSettingsWidget::tr("Bare Metal"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new DebugServerProvidersSettingsWidget; });
}

} // namespace Internal
} // namespace BareMetal
