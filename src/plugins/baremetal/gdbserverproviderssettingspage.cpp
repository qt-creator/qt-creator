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
#include <QSignalMapper>
#include <QSpacerItem>
#include <QTextStream>
#include <QTreeView>
#include <QVBoxLayout>

namespace BareMetal {
namespace Internal {

class GdbServerProviderNode
{
public:
    explicit GdbServerProviderNode(GdbServerProviderNode *parent,
                                   GdbServerProvider *provider = 0,
                                   bool changed = false);
    ~GdbServerProviderNode();

    GdbServerProviderNode *parent;
    QList<GdbServerProviderNode *> childNodes;
    GdbServerProvider *provider;
    GdbServerProviderConfigWidget *widget;
    bool changed;
};

GdbServerProviderNode::GdbServerProviderNode(
        GdbServerProviderNode *parent,
        GdbServerProvider *provider,
        bool changed)
    : parent(parent)
    , provider(provider)
    , changed(changed)
{
    if (parent)
        parent->childNodes.append(this);

    widget = provider ? provider->configurationWidget() : 0;
}

GdbServerProviderNode::~GdbServerProviderNode()
{
    // Do not delete provider, we do not own it.

    for (int i = childNodes.size(); --i >= 0; ) {
        GdbServerProviderNode *child = childNodes.at(i);
        child->parent = 0;
        delete child;
    }

    if (parent)
        parent->childNodes.removeOne(this);
}

GdbServerProviderModel::GdbServerProviderModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_root(new GdbServerProviderNode(0))
{
    const GdbServerProviderManager *manager = GdbServerProviderManager::instance();

    connect(manager, &GdbServerProviderManager::providerAdded,
            this, &GdbServerProviderModel::addProvider);
    connect(manager, &GdbServerProviderManager::providerRemoved,
            this, &GdbServerProviderModel::removeProvider);

    foreach (GdbServerProvider *p, manager->providers())
        addProvider(p);
}

GdbServerProviderModel::~GdbServerProviderModel()
{
    delete m_root;
}

QModelIndex GdbServerProviderModel::index(
        int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row >= 0 && row < m_root->childNodes.count())
            return createIndex(row, column, m_root->childNodes.at(row));
    }

    return QModelIndex();
}

QModelIndex GdbServerProviderModel::index(
        const QModelIndex &topIdx, const GdbServerProvider *provider) const
{
    const GdbServerProviderNode *current =
            topIdx.isValid() ? nodeFromIndex(topIdx) : m_root;
    QTC_ASSERT(current, return QModelIndex());

    if (current->provider == provider)
        return topIdx;

    for (int i = 0; i < current->childNodes.count(); ++i) {
        const QModelIndex idx = index(index(current->childNodes.at(i)), provider);
        if (idx.isValid())
            return idx;
    }
    return QModelIndex();
}

QModelIndex GdbServerProviderModel::parent(const QModelIndex &idx) const
{
    const GdbServerProviderNode *n = nodeFromIndex(idx);
    return (n->parent == m_root) ? QModelIndex() : index(n->parent);
}

int GdbServerProviderModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_root->childNodes.count();

    const GdbServerProviderNode *n = nodeFromIndex(parent);
    return n->childNodes.count();
}

int GdbServerProviderModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ColumnsCount;
}

QVariant GdbServerProviderModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const GdbServerProviderNode *n = nodeFromIndex(index);
    QTC_ASSERT(n, return QVariant());

    if (!n->provider)
        return QVariant();

    if (role == Qt::FontRole) {
        QFont f = QApplication::font();
        if (n->changed)
            f.setBold(true);
        return f;
    } else if (role == Qt::DisplayRole) {
        return (index.column() == NameIndex)
                ? n->provider->displayName()
                : n->provider->typeDisplayName();
    }

    // FIXME: Need to handle also and ToolTipRole role?

    return QVariant();
}

Qt::ItemFlags GdbServerProviderModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    const GdbServerProviderNode *n = nodeFromIndex(index);
    Q_ASSERT(n);
    return (n->provider) ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable)
                         : Qt::ItemIsEnabled;
}

QVariant GdbServerProviderModel::headerData(
        int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return section == NameIndex ? tr("Name") : tr("Type");
    return QVariant();
}

GdbServerProvider *GdbServerProviderModel::provider(
        const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    const GdbServerProviderNode *n = nodeFromIndex(index);
    Q_ASSERT(n);
    return n->provider;
}

GdbServerProviderConfigWidget *GdbServerProviderModel::widget(
        const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    const GdbServerProviderNode *n = nodeFromIndex(index);
    Q_ASSERT(n);
    return n->widget;
}

bool GdbServerProviderModel::isDirty() const
{
    return Utils::anyOf(m_root->childNodes, [](GdbServerProviderNode *n) {
        return n->changed;
    });
}

bool GdbServerProviderModel::isDirty(GdbServerProvider *p) const
{
    return Utils::anyOf(m_root->childNodes, [p](GdbServerProviderNode *n) {
        return n->provider == p && n->changed;
    });
}

void GdbServerProviderModel::setDirty()
{
    const auto w = qobject_cast<GdbServerProviderConfigWidget *>(sender());
    foreach (GdbServerProviderNode *n, m_root->childNodes) {
        if (n->widget == w) {
            n->changed = true;
            emit dataChanged(index(n, 0), index(n, columnCount(QModelIndex())));
        }
    }
}

void GdbServerProviderModel::apply()
{
    // Remove unused providers
    foreach (const GdbServerProviderNode *n, m_toRemoveNodes) {
        Q_ASSERT(!n->parent);
        GdbServerProviderManager::instance()->deregisterProvider(n->provider);
    }
    Q_ASSERT(m_toRemoveNodes.isEmpty());

    // Update providers
    foreach (GdbServerProviderNode *n, m_root->childNodes) {
        Q_ASSERT(n);

        if (!n->changed)
            continue;

        Q_ASSERT(n->provider);
        if (n->widget)
            n->widget->apply();

        n->changed = false;

        emit dataChanged(index(n, 0), index(n, columnCount(QModelIndex())));
    }

    // Add new (and already updated) providers
    QStringList removedProviders;
    foreach (const GdbServerProviderNode *n, m_toAddNodes) {
        if (!GdbServerProviderManager::instance()->registerProvider(n->provider))
            removedProviders << n->provider->displayName();
    }

    qDeleteAll(m_toAddNodes);

    if (removedProviders.count() == 1) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             tr("Duplicate Providers Detected"),
                             tr("The following provider was already configured:<br>"
                                "&nbsp;%1<br>"
                                "It was not configured again.")
                             .arg(removedProviders.at(0)));

    } else if (!removedProviders.isEmpty()) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             tr("Duplicate Providers Detected"),
                             tr("The following providers were already configured:<br>"
                                "&nbsp;%1<br>"
                                "They were not configured again.")
                             .arg(removedProviders.join(QLatin1String(",<br>&nbsp;"))));
    }
}

void GdbServerProviderModel::markForRemoval(GdbServerProvider *provider)
{
    GdbServerProviderNode *n = findNode(m_root->childNodes, provider);
    QTC_ASSERT(n, return);

    const int row = m_root->childNodes.indexOf(n);
    emit beginRemoveRows(index(m_root), row, row);
    m_root->childNodes.removeOne(n);
    n->parent = 0;

    if (m_toAddNodes.contains(n)) {
        delete n->provider;
        n->provider = 0;
        m_toAddNodes.removeOne(n);
        delete n;
    } else {
        m_toRemoveNodes.append(n);
    }

    emit endRemoveRows();
}

void GdbServerProviderModel::markForAddition(GdbServerProvider *provider)
{
    const int pos = m_root->childNodes.size();
    emit beginInsertRows(index(m_root), pos, pos);
    GdbServerProviderNode *n = createNode(m_root, provider, true);
    m_toAddNodes.append(n);
    emit endInsertRows();
}

QModelIndex GdbServerProviderModel::index(
        GdbServerProviderNode *n, int column) const
{
    if (n == m_root)
        return QModelIndex();

    if (n->parent == m_root)
        return index(m_root->childNodes.indexOf(n), column, QModelIndex());

    return index(n->parent->childNodes.indexOf(n),
                 column, index(n->parent));
}

GdbServerProviderNode *GdbServerProviderModel::createNode(
        GdbServerProviderNode *parent,
        GdbServerProvider *provider, bool changed)
{
    auto n = new GdbServerProviderNode(parent, provider, changed);
    if (n->widget) {
        connect(n->widget, &GdbServerProviderConfigWidget::dirty,
                this, &GdbServerProviderModel::setDirty);
    }
    return n;
}

GdbServerProviderNode *GdbServerProviderModel::nodeFromIndex(
        const QModelIndex &index) const
{
    return static_cast<GdbServerProviderNode *>(index.internalPointer());
}

GdbServerProviderNode *GdbServerProviderModel::findNode(
        const QList<GdbServerProviderNode *> &container,
        const GdbServerProvider *provider)
{
    return Utils::findOrDefault(container, [provider](GdbServerProviderNode *n) {
        return n->provider == provider;
    });
}

void GdbServerProviderModel::addProvider(GdbServerProvider *provider)
{
    GdbServerProviderNode *n = findNode(m_toAddNodes, provider);
    if (n) {
        m_toAddNodes.removeOne(n);
        // do not delete n: Still used elsewhere!
        return;
    }

    const int row = m_root->childNodes.count();

    beginInsertRows(index(m_root), row, row);
    createNode(m_root, provider, false);
    endInsertRows();

    emit providerStateChanged();
}

void GdbServerProviderModel::removeProvider(GdbServerProvider *provider)
{
    GdbServerProviderNode *n = findNode(m_toRemoveNodes, provider);
    if (n) {
        m_toRemoveNodes.removeOne(n);
        delete n;
        return;
    }

    int row = 0;
    foreach (GdbServerProviderNode *current, m_root->childNodes) {
        if (current->provider == provider) {
            n = current;
            break;
        }
        ++row;
    }

    beginRemoveRows(index(m_root), row, row);
    m_root->childNodes.removeAt(row);
    delete n;
    endRemoveRows();

    emit providerStateChanged();
}

GdbServerProvidersSettingsPage::GdbServerProvidersSettingsPage(
        QObject *parent)
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
    if (!m_configWidget) {
        // Actual page setup:
        m_configWidget = new QWidget;

        m_providerView = new QTreeView(m_configWidget);
        m_providerView->setUniformRowHeights(true);
        m_providerView->header()->setStretchLastSection(false);

        m_addButton = new QPushButton(tr("Add"), m_configWidget);
        m_cloneButton = new QPushButton(tr("Clone"), m_configWidget);
        m_delButton = new QPushButton(tr("Remove"), m_configWidget);

        m_container = new Utils::DetailsWidget(m_configWidget);
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

        auto horizontalLayout = new QHBoxLayout(m_configWidget);
        horizontalLayout->addLayout(verticalLayout);
        horizontalLayout->addWidget(m_container);
        Q_ASSERT(!m_model);
        m_model = new GdbServerProviderModel(m_configWidget);

        connect(m_model, &GdbServerProviderModel::providerStateChanged,
                this, &GdbServerProvidersSettingsPage::updateState);

        m_providerView->setModel(m_model);

        auto headerView = m_providerView->header();
        headerView->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        headerView->setSectionResizeMode(1, QHeaderView::Stretch);
        m_providerView->expandAll();

        m_selectionModel = m_providerView->selectionModel();

        connect(m_selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                SLOT(providerSelectionChanged()));

        connect(GdbServerProviderManager::instance(), &GdbServerProviderManager::providersChanged,
                this, &GdbServerProvidersSettingsPage::providerSelectionChanged);

        // Set up add menu:
        auto addMenu = new QMenu(m_addButton);
        auto mapper = new QSignalMapper(addMenu);
        connect(mapper, SIGNAL(mapped(QObject*)), SLOT(createProvider(QObject*)));

        foreach (const auto f, GdbServerProviderManager::instance()->factories()) {
            auto action = new QAction(addMenu);
            action->setText(f->displayName());
            connect(action, SIGNAL(triggered()), mapper, SLOT(map()));
            mapper->setMapping(action, static_cast<QObject *>(f));
            addMenu->addAction(action);
        }

        connect(m_cloneButton, SIGNAL(clicked()), mapper, SLOT(map()));
        mapper->setMapping(m_cloneButton, static_cast<QObject *>(0));

        m_addButton->setMenu(addMenu);

        connect(m_delButton, &QPushButton::clicked,
                this, &GdbServerProvidersSettingsPage::removeProvider);

        updateState();
    }

    return m_configWidget;
}

void GdbServerProvidersSettingsPage::apply()
{
    if (m_model)
        m_model->apply();
}

void GdbServerProvidersSettingsPage::finish()
{
    disconnect(GdbServerProviderManager::instance(), SIGNAL(providersChanged()),
               this, SLOT(providerSelectionChanged()));

    delete m_configWidget;
}

void GdbServerProvidersSettingsPage::providerSelectionChanged()
{
    if (!m_container)
        return;
    const QModelIndex current = currentIndex();
    QWidget *w = m_container->takeWidget(); // Prevent deletion.
    if (w)
        w->setVisible(false);
    w = current.isValid() ? m_model->widget(current) : 0;
    m_container->setWidget(w);
    m_container->setVisible(w != 0);
    updateState();
}

void GdbServerProvidersSettingsPage::createProvider(QObject *factoryObject)
{
    GdbServerProvider *provider = 0;

    auto f = static_cast<GdbServerProviderFactory *>(factoryObject);
    if (!f) {
        GdbServerProvider *old = m_model->provider(currentIndex());
        if (!old)
            return;
        provider = old->clone();
    } else {
        provider = f->create();
    } if (!provider) {
        return;
    }

    m_model->markForAddition(provider);

    const QModelIndex newIdx = m_model->index(QModelIndex(), provider);
    m_selectionModel->select(newIdx,
                             QItemSelectionModel::Clear
                             | QItemSelectionModel::SelectCurrent
                             | QItemSelectionModel::Rows);
}

void GdbServerProvidersSettingsPage::removeProvider()
{
    GdbServerProvider *p = m_model->provider(currentIndex());
    if (!p)
        return;
    m_model->markForRemoval(p);
}

void GdbServerProvidersSettingsPage::updateState()
{
    if (!m_cloneButton)
        return;

    bool canCopy = false;
    bool canDelete = false;
    const GdbServerProvider *p = m_model->provider(currentIndex());
    if (p) {
        canCopy = p->isValid();
        canDelete = true;
    }

    m_cloneButton->setEnabled(canCopy);
    m_delButton->setEnabled(canDelete);
}

QModelIndex GdbServerProvidersSettingsPage::currentIndex() const
{
    if (!m_selectionModel)
        return QModelIndex();

    const QModelIndexList rows = m_selectionModel->selectedRows();
    if (rows.count() != 1)
        return QModelIndex();
    return rows.at(0);
}

} // namespace Internal
} // namespace BareMetal
