/****************************************************************************
**
** Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gdbserverproviderssettingspage.h"
#include "gdbserverprovider.h"
#include "baremetalconstants.h"
#include "gdbserverprovidermanager.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/detailswidget.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QApplication>
#include <QAction>
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

using namespace Utils;

namespace BareMetal {
namespace Internal {

class GdbServerProviderNode : public TreeItem
{
public:
    GdbServerProviderNode(GdbServerProvider *provider, bool changed = false)
        : provider(provider), changed(changed)
    {
        widget = provider ? provider->configurationWidget() : 0;
    }

    Qt::ItemFlags flags(int) const override
    {
        return provider ? Qt::ItemIsEnabled|Qt::ItemIsSelectable : Qt::ItemIsEnabled;
    }

    QVariant data(int column, int role) const override
    {
        if (!provider)
            return QVariant();

        if (role == Qt::FontRole) {
            QFont f = QApplication::font();
            if (changed)
                f.setBold(true);
            return f;
        }

        if (role == Qt::DisplayRole) {
            return column == 0 ? provider->displayName() : provider->typeDisplayName();
        }

        // FIXME: Need to handle ToolTipRole role?
        return QVariant();
    }

    GdbServerProvider *provider;
    GdbServerProviderConfigWidget *widget;
    bool changed;
};


GdbServerProviderModel::GdbServerProviderModel(QObject *parent)
    : TreeModel(parent)
{
    setHeader({tr("Name"), tr("Type")});

    const GdbServerProviderManager *manager = GdbServerProviderManager::instance();

    connect(manager, &GdbServerProviderManager::providerAdded,
            this, &GdbServerProviderModel::addProvider);
    connect(manager, &GdbServerProviderManager::providerRemoved,
            this, &GdbServerProviderModel::removeProvider);

    foreach (GdbServerProvider *p, manager->providers())
        addProvider(p);
}

GdbServerProvider *GdbServerProviderModel::provider(const QModelIndex &index) const
{
    if (GdbServerProviderNode *node = nodeForIndex(index))
        return node->provider;

    return 0;
}

GdbServerProviderNode *GdbServerProviderModel::nodeForIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return static_cast<GdbServerProviderNode *>(itemForIndex(index));
}

void GdbServerProviderModel::apply()
{
    // Remove unused providers
    foreach (GdbServerProvider *provider, m_providersToRemove)
        GdbServerProviderManager::instance()->deregisterProvider(provider);
    QTC_ASSERT(m_providersToRemove.isEmpty(), m_providersToRemove.clear());

    // Update providers
    foreach (TreeItem *item, rootItem()->children()) {
        auto n = static_cast<GdbServerProviderNode *>(item);
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
    foreach (GdbServerProvider *provider, m_providersToAdd) {
        if (!GdbServerProviderManager::instance()->registerProvider(provider))
            skippedProviders << provider->displayName();
    }

    m_providersToAdd.clear();

    if (!skippedProviders.isEmpty()) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             tr("Duplicate Providers Detected"),
                             tr("The following providers were already configured:<br>"
                                "&nbsp;%1<br>"
                                "They were not configured again.")
                             .arg(skippedProviders.join(QLatin1String(",<br>&nbsp;"))));
    }
}

GdbServerProviderNode *GdbServerProviderModel::findNode(const GdbServerProvider *provider) const
{
    auto test = [provider](TreeItem *item) {
        return static_cast<GdbServerProviderNode *>(item)->provider == provider;
    };

    return static_cast<GdbServerProviderNode *>(Utils::findOrDefault(rootItem()->children(), test));
}

QModelIndex GdbServerProviderModel::indexForProvider(GdbServerProvider *provider) const
{
    GdbServerProviderNode *n = findNode(provider);
    return n ? indexForItem(n) : QModelIndex();
}

void GdbServerProviderModel::markForRemoval(GdbServerProvider *provider)
{
    GdbServerProviderNode *n = findNode(provider);
    QTC_ASSERT(n, return);
    delete takeItem(n);

    if (m_providersToAdd.contains(provider)) {
        m_providersToAdd.removeOne(provider);
        delete provider;
    } else {
        m_providersToRemove.append(provider);
    }
}

void GdbServerProviderModel::markForAddition(GdbServerProvider *provider)
{
    GdbServerProviderNode *n = createNode(provider, true);
    rootItem()->appendChild(n);
    m_providersToAdd.append(provider);
}

GdbServerProviderNode *GdbServerProviderModel::createNode(
        GdbServerProvider *provider, bool changed)
{
    auto n = new GdbServerProviderNode(provider, changed);
    if (n->widget) {
        connect(n->widget, &GdbServerProviderConfigWidget::dirty, this, [this, n] {
            foreach (TreeItem *item, rootItem()->children()) {
                auto nn = static_cast<GdbServerProviderNode *>(item);
                if (nn->widget == n->widget) {
                    nn->changed = true;
                    nn->update();
                }
            }
        });
    }
    return n;
}

void GdbServerProviderModel::addProvider(GdbServerProvider *provider)
{
    if (findNode(provider))
        m_providersToAdd.removeOne(provider);
    else
        rootItem()->appendChild(createNode(provider, false));

    emit providerStateChanged();
}

void GdbServerProviderModel::removeProvider(GdbServerProvider *provider)
{
    m_providersToRemove.removeAll(provider);
    if (GdbServerProviderNode *n = findNode(provider))
        delete takeItem(n);

    emit providerStateChanged();
}

class GdbServerProvidersSettingsWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(BareMetal::Internal::GdbServerProvidersSettingsPage)

public:
    GdbServerProvidersSettingsWidget(GdbServerProvidersSettingsPage *page);

    void providerSelectionChanged();
    void removeProvider();
    void updateState();

    void createProvider(GdbServerProviderFactory *f);
    QModelIndex currentIndex() const;

public:
    GdbServerProvidersSettingsPage *m_page;
    GdbServerProviderModel m_model;
    QItemSelectionModel *m_selectionModel;
    QTreeView *m_providerView;
    Utils::DetailsWidget *m_container;
    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_delButton;
};

GdbServerProvidersSettingsWidget::GdbServerProvidersSettingsWidget
        (GdbServerProvidersSettingsPage *page)
    : m_page(page)
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

    auto buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(6);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_cloneButton);
    buttonLayout->addWidget(m_delButton);
    auto spacerItem = new QSpacerItem(40, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);
    buttonLayout->addItem(spacerItem);

    auto verticalLayout = new QVBoxLayout();
    verticalLayout->addWidget(m_providerView);
    verticalLayout->addLayout(buttonLayout);

    auto horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->addLayout(verticalLayout);
    horizontalLayout->addWidget(m_container);

    connect(&m_model, &GdbServerProviderModel::providerStateChanged,
            this, &GdbServerProvidersSettingsWidget::updateState);

    m_providerView->setModel(&m_model);

    auto headerView = m_providerView->header();
    headerView->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    headerView->setSectionResizeMode(1, QHeaderView::Stretch);
    m_providerView->expandAll();

    m_selectionModel = m_providerView->selectionModel();

    connect(m_selectionModel, &QItemSelectionModel::selectionChanged,
            this, &GdbServerProvidersSettingsWidget::providerSelectionChanged);

    connect(GdbServerProviderManager::instance(), &GdbServerProviderManager::providersChanged,
            this, &GdbServerProvidersSettingsWidget::providerSelectionChanged);

    // Set up add menu:
    auto addMenu = new QMenu(m_addButton);

    foreach (const auto f, GdbServerProviderManager::instance()->factories()) {
        auto action = new QAction(addMenu);
        action->setText(f->displayName());
        connect(action, &QAction::triggered, this, [this, f] { createProvider(f); });
        addMenu->addAction(action);
    }

    connect(m_cloneButton, &QAbstractButton::clicked, this, [this] { createProvider(0); });

    m_addButton->setMenu(addMenu);

    connect(m_delButton, &QPushButton::clicked,
            this, &GdbServerProvidersSettingsWidget::removeProvider);

    updateState();
}

void GdbServerProvidersSettingsWidget::providerSelectionChanged()
{
    if (!m_container)
        return;
    const QModelIndex current = currentIndex();
    QWidget *w = m_container->takeWidget(); // Prevent deletion.
    if (w)
        w->setVisible(false);

    GdbServerProviderNode *node = m_model.nodeForIndex(current);
    w = node ? node->widget : 0;
    m_container->setWidget(w);
    m_container->setVisible(w != 0);
    updateState();
}

void GdbServerProvidersSettingsWidget::createProvider(GdbServerProviderFactory *f)
{
    GdbServerProvider *provider = 0;
    if (!f) {
        GdbServerProvider *old = m_model.provider(currentIndex());
        if (!old)
            return;
        provider = old->clone();
    } else {
        provider = f->create();
    }

    if (!provider)
        return;

    m_model.markForAddition(provider);

    m_selectionModel->select(m_model.indexForProvider(provider),
                             QItemSelectionModel::Clear
                             | QItemSelectionModel::SelectCurrent
                             | QItemSelectionModel::Rows);
}

void GdbServerProvidersSettingsWidget::removeProvider()
{
    if (GdbServerProvider *p = m_model.provider(currentIndex()))
        m_model.markForRemoval(p);
}

void GdbServerProvidersSettingsWidget::updateState()
{
    if (!m_cloneButton)
        return;

    bool canCopy = false;
    bool canDelete = false;
    if (const GdbServerProvider *p = m_model.provider(currentIndex())) {
        canCopy = p->isValid();
        canDelete = true;
    }

    m_cloneButton->setEnabled(canCopy);
    m_delButton->setEnabled(canDelete);
}

QModelIndex GdbServerProvidersSettingsWidget::currentIndex() const
{
    if (!m_selectionModel)
        return QModelIndex();

    const QModelIndexList rows = m_selectionModel->selectedRows();
    if (rows.count() != 1)
        return QModelIndex();
    return rows.at(0);
}


GdbServerProvidersSettingsPage::GdbServerProvidersSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
    setCategory(Constants::BAREMETAL_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate(
                       "BareMetal", Constants::BAREMETAL_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::BAREMETAL_SETTINGS_CATEGORY_ICON));
    setId(Constants::GDB_PROVIDERS_SETTINGS_ID);
    setDisplayName(tr("GDB Server Providers"));
}

QWidget *GdbServerProvidersSettingsPage::widget()
{
    if (!m_configWidget)
        m_configWidget = new GdbServerProvidersSettingsWidget(this);
     return m_configWidget;
}

void GdbServerProvidersSettingsPage::apply()
{
    if (m_configWidget)
        m_configWidget->m_model.apply();
}

void GdbServerProvidersSettingsPage::finish()
{
    if (m_configWidget)
        disconnect(GdbServerProviderManager::instance(), &GdbServerProviderManager::providersChanged,
                   m_configWidget.data(), &GdbServerProvidersSettingsWidget::providerSelectionChanged);

    delete m_configWidget.data();
}

} // namespace Internal
} // namespace BareMetal
