// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pluginview.h"

#include "extensionsystemtr.h"
#include "pluginmanager.h"
#include "pluginspec.h"

#include <utils/algorithm.h>
#include <utils/categorysortfiltermodel.h>
#include <utils/utilsicons.h>
#include <utils/itemviews.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QGridLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QSet>

/*!
    \class ExtensionSystem::PluginView
    \inheaderfile extensionsystem/pluginview.h
    \inmodule QtCreator

    \brief The PluginView class implements a widget that shows a list of all
    plugins and their state.

    This class can be embedded for example in a dialog in the application that
    uses the plugin manager.
    The class also provides notifications for interaction with the list.

    \sa ExtensionSystem::PluginDetailsView
    \sa ExtensionSystem::PluginErrorView
*/

/*!
    \fn void ExtensionSystem::PluginView::currentPluginChanged(ExtensionSystem::PluginSpec *spec)
    The current selection in the plugin list has changed to the
    plugin corresponding to \a spec.
*/

/*!
    \fn void ExtensionSystem::PluginView::pluginActivated(ExtensionSystem::PluginSpec *spec)
    The plugin list entry corresponding to \a spec has been activated,
    for example by a double-click.
*/

/*!
    \fn void ExtensionSystem::PluginView::pluginsChanged(const QSet<ExtensionSystem::PluginSpec *> &spec, bool enabled)
    The value of \a enabled for the plugin list entry corresponding to \a spec
    changed.
*/

using namespace Utils;

namespace ExtensionSystem {

namespace Internal {

enum Columns { NameColumn, LoadedColumn, VersionColumn, VendorColumn, };

enum IconIndex { OkIcon, ErrorIcon, NotLoadedIcon };

static const int SortRole = Qt::UserRole + 1;

static const QIcon &icon(IconIndex icon)
{
    switch (icon) {
    case OkIcon: {
        static const QIcon ok = Utils::Icons::OK.icon();
        return ok;
    }
    case ErrorIcon: {
        static const QIcon error = Utils::Icons::BROKEN.icon();
        return error;
    }
    default:
    case NotLoadedIcon: {
        static const QIcon notLoaded = Utils::Icons::NOTLOADED.icon();
        return notLoaded;
    }
    }
}

class PluginItem : public TreeItem
{
public:
    PluginItem(PluginSpec *spec, PluginData *data)
        : m_spec(spec), m_data(data)
    {}

    QVariant data(int column, int role) const override
    {
        switch (column) {
        case NameColumn: {
            const QString displayName = m_spec->displayName();
            if (role == Qt::DisplayRole) {
                QStringList decoration;
                if (m_spec->isDeprecated())
                    decoration.append(Tr::tr("deprecated"));
                if (m_spec->isExperimental())
                    decoration.append(Tr::tr("experimental"));

                if (decoration.isEmpty())
                    return displayName;
                return QString::fromLatin1("%1 (%2)")
                    .arg(displayName, decoration.join(QLatin1Char(',')));
            }
            if (role == SortRole)
                return displayName;
            if (role == Qt::ToolTipRole) {
                QString toolTip;
                if (!m_spec->isAvailableForHostPlatform())
                    toolTip = Tr::tr("Path: %1\nPlugin is not available on this platform.");
                else if (m_spec->isEnabledIndirectly())
                    toolTip = Tr::tr(
                        "Path: %1\nPlugin is enabled as dependency of an enabled plugin.");
                else if (m_spec->isForceEnabled())
                    toolTip = Tr::tr("Path: %1\nPlugin is enabled by command line argument.");
                else if (m_spec->isForceDisabled())
                    toolTip = Tr::tr("Path: %1\nPlugin is disabled by command line argument.");
                else
                    toolTip = Tr::tr("Path: %1");
                return toolTip.arg(m_spec->filePath().toUserOutput());
            }
            if (role == Qt::DecorationRole) {
                bool ok = !m_spec->hasError();
                QIcon i = icon(ok ? OkIcon : ErrorIcon);
                if (ok && m_spec->state() != PluginSpec::Running)
                    i = icon(NotLoadedIcon);
                return i;
            }
            break;
        }

        case LoadedColumn:
            if (!m_spec->isAvailableForHostPlatform()) {
                if (role == Qt::CheckStateRole || role == SortRole)
                    return Qt::Unchecked;
                if (role == Qt::ToolTipRole)
                    return Tr::tr("Plugin is not available on this platform.");
            } else if (m_spec->isRequired()) {
                if (role == Qt::CheckStateRole || role == SortRole)
                    return Qt::Checked;
                if (role == Qt::ToolTipRole)
                    return Tr::tr("Plugin is required.");
            } else {
                if (role == Qt::CheckStateRole || role == SortRole)
                    return m_spec->isEnabledBySettings() ? Qt::Checked : Qt::Unchecked;
                if (role == Qt::ToolTipRole)
                    return Tr::tr("Load on startup");
            }
            break;

        case VersionColumn:
            if (role == Qt::DisplayRole || role == SortRole)
                return QString::fromLatin1("%1 (%2)").arg(m_spec->version(), m_spec->compatVersion());
            break;

        case VendorColumn:
            if (role == Qt::DisplayRole || role == SortRole)
                return m_spec->vendor();
            break;
        }

        return QVariant();
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        if (column == LoadedColumn && role == Qt::CheckStateRole)
            return m_data->setPluginsEnabled({m_spec}, data.toBool());
        return false;
    }

    bool isEnabled() const
    {
        return m_spec->isAvailableForHostPlatform() && !m_spec->isRequired();
    }

    Qt::ItemFlags flags(int column) const override
    {
        Qt::ItemFlags ret = Qt::ItemIsSelectable;

        if (isEnabled())
            ret |= Qt::ItemIsEnabled;

        if (column == LoadedColumn) {
            if (m_spec->isAvailableForHostPlatform() && !m_spec->isRequired())
                ret |= Qt::ItemIsUserCheckable;
        }

        return ret;
    }

public:
    PluginSpec *m_spec; // Not owned.
    PluginData *m_data; // Not owned.
};

class CollectionItem : public TreeItem
{
public:
    CollectionItem(const QString &name, const PluginSpecs &plugins, PluginData *data)
        : m_name(name)
        , m_plugins(plugins)
        , m_data(data)
    {
        for (PluginSpec *spec : plugins)
            appendChild(new PluginItem(spec, data));
    }

    QVariant data(int column, int role) const override
    {
        if (column == NameColumn) {
            if (role == Qt::DisplayRole || role == SortRole)
                return m_name;
        }

        if (column == LoadedColumn) {
            if (role == Qt::ToolTipRole)
                return Tr::tr("Load on Startup");
            if (role == Qt::CheckStateRole || role == SortRole) {
                int checkedCount = 0;
                for (PluginSpec *spec : m_plugins) {
                    if (spec->isEnabledBySettings())
                        ++checkedCount;
                }

                if (checkedCount == 0)
                    return Qt::Unchecked;
                if (checkedCount == m_plugins.length())
                    return Qt::Checked;
                return Qt::PartiallyChecked;
            }
        }

        return QVariant();
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        if (column == LoadedColumn && role == Qt::CheckStateRole) {
            const PluginSpecs affectedPlugins
                = Utils::filtered(m_plugins, [](PluginSpec *spec) { return !spec->isRequired(); });
            if (m_data->setPluginsEnabled(toSet(affectedPlugins), data.toBool())) {
                update();
                return true;
            }
        }
        return false;
    }

    Qt::ItemFlags flags(int column) const override
    {
        Qt::ItemFlags ret = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        if (column == LoadedColumn)
            ret |= Qt::ItemIsUserCheckable;
        return ret;
    }

public:
    QString m_name;
    const PluginSpecs m_plugins;
    PluginData *m_data; // Not owned.
};

} // Internal

using namespace ExtensionSystem::Internal;

PluginData::PluginData(QWidget *parent, PluginView *owner)
    : m_parent(parent), m_pluginView(owner)
{
    m_model = new TreeModel<TreeItem, CollectionItem, PluginItem>(parent);
    m_model->setHeader({ Tr::tr("Name"), Tr::tr("Load"), Tr::tr("Version"), Tr::tr("Vendor") });

    m_sortModel = new CategorySortFilterModel(parent);
    m_sortModel->setSourceModel(m_model);
    m_sortModel->setSortRole(SortRole);
    m_sortModel->setFilterKeyColumn(-1/*all*/);
}

/*!
    Constructs a plugin view with \a parent that displays a list of plugins
    from a plugin manager.
*/
PluginView::PluginView(QWidget *parent)
    : QWidget(parent), m_data(this, this)
{
    m_categoryView = new TreeView(this);
    m_categoryView->setAlternatingRowColors(true);
    m_categoryView->setIndentation(20);
    m_categoryView->setSortingEnabled(true);
    m_categoryView->setActivationMode(DoubleClickActivation);
    m_categoryView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_categoryView->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_categoryView->setModel(m_data.m_sortModel);

    auto *gridLayout = new QGridLayout(this);
    gridLayout->setContentsMargins(2, 2, 2, 2);
    gridLayout->addWidget(m_categoryView, 1, 0, 1, 1);

    QHeaderView *header = m_categoryView->header();
    header->setSortIndicator(NameColumn, Qt::AscendingOrder);
    header->setSectionResizeMode(QHeaderView::ResizeToContents);

    connect(PluginManager::instance(), &PluginManager::pluginsChanged,
            this, &PluginView::updatePlugins);

    connect(m_categoryView, &QAbstractItemView::activated, this,
            [this](const QModelIndex &idx) { emit pluginActivated(pluginForIndex(idx)); });

    connect(m_categoryView->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex &idx) { emit currentPluginChanged(pluginForIndex(idx)); });

    updatePlugins();
}

/*!
    \internal
*/
PluginView::~PluginView() = default;

/*!
    Returns the current selection in the list of plugins.
*/
PluginSpec *PluginView::currentPlugin() const
{
    return pluginForIndex(m_categoryView->currentIndex());
}

/*!
    Sets the \a filter for listing plugins.
*/
void PluginView::setFilter(const QString &filter)
{
    m_data.m_sortModel->setFilterRegularExpression(
        QRegularExpression(QRegularExpression::escape(filter),
                           QRegularExpression::CaseInsensitiveOption));
    m_categoryView->expandAll();
}

PluginSpec *PluginView::pluginForIndex(const QModelIndex &index) const
{
    const QModelIndex &sourceIndex = m_data.m_sortModel->mapToSource(index);
    PluginItem *item = m_data.m_model->itemForIndexAtLevel<2>(sourceIndex);
    return item ? item->m_spec: nullptr;
}

void PluginView::updatePlugins()
{
    // Model.
    m_data.m_model->clear();

    const QHash<QString, PluginSpecs> pluginCollections
        = PluginManager::pluginCollections();
    std::vector<CollectionItem *> collections;
    const auto end = pluginCollections.cend();
    for (auto it = pluginCollections.cbegin(); it != end; ++it) {
        const QString name = it.key().isEmpty() ? Tr::tr("Utilities") : it.key();
        collections.push_back(new CollectionItem(name, it.value(), &m_data));
    }
    Utils::sort(collections, &CollectionItem::m_name);

    for (CollectionItem *collection : std::as_const(collections))
        m_data.m_model->rootItem()->appendChild(collection);

    emit m_data.m_model->layoutChanged();
    m_categoryView->expandAll();
}

PluginData &PluginView::data()
{
    return m_data;
}

bool PluginData::setPluginsEnabled(const QSet<PluginSpec *> &plugins, bool enable)
{
    std::optional<QSet<PluginSpec *>> additionalPlugins
        = PluginManager::askForEnablingPlugins(m_parent, plugins, enable);
    if (!additionalPlugins) // canceled
        return false;

    const QSet<PluginSpec *> affectedPlugins = plugins + *additionalPlugins;
    for (PluginSpec *spec : affectedPlugins) {
        PluginItem *item = m_model->findItemAtLevel<2>([spec](PluginItem *item) {
                return item->m_spec == spec;
        });
        QTC_ASSERT(item, continue);
        if (m_affectedPlugins.find(spec) == m_affectedPlugins.end())
            m_affectedPlugins[spec] = spec->isEnabledBySettings();
        spec->setEnabledBySettings(enable);
        item->updateColumn(LoadedColumn);
        item->parent()->updateColumn(LoadedColumn);
    }

    if (m_pluginView)
        emit m_pluginView->pluginsChanged(affectedPlugins, enable);

    return true;
}

void PluginView::cancelChanges()
{
    for (auto element : m_data.m_affectedPlugins)
        element.first->setEnabledBySettings(element.second);
}

} // namespace ExtensionSystem
