/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "pluginview.h"
#include "pluginview_p.h"
#include "pluginmanager.h"
#include "pluginspec.h"
#include "plugincollection.h"
#include "ui_pluginview.h"

#include <QtCore/QDir>
#include <QtGui/QHeaderView>
#include <QtGui/QTreeWidgetItem>
#include <QtGui/QPalette>

#include <QtDebug>

/*!
    \class ExtensionSystem::PluginView
    \brief Widget that shows a list of all plugins and their state.

    This can be embedded e.g. in a dialog in the application that
    uses the plugin manager.
    The class also provides notifications for interactions with the list.

    \sa ExtensionSystem::PluginDetailsView
    \sa ExtensionSystem::PluginErrorView
*/

/*!
    \fn void PluginView::currentPluginChanged(ExtensionSystem::PluginSpec *spec)
    The current selection in the plugin list has changed to the
    plugin corresponding to \a spec.
*/

/*!
    \fn void PluginView::pluginActivated(ExtensionSystem::PluginSpec *spec)
    The plugin list entry corresponding to \a spec has been activated,
    e.g. by a double-click.
*/

using namespace ExtensionSystem;

Q_DECLARE_METATYPE(ExtensionSystem::PluginSpec*);
Q_DECLARE_METATYPE(ExtensionSystem::PluginCollection*);

/*!
    \fn PluginView::PluginView(PluginManager *manager, QWidget *parent)
    Constructs a PluginView that gets the list of plugins from the
    given plugin \a manager with a given \a parent widget.
*/
PluginView::PluginView(PluginManager *manager, QWidget *parent)
    : QWidget(parent),
      m_ui(new Internal::Ui::PluginView),
      p(new Internal::PluginViewPrivate),
      m_allowCheckStateUpdate(true),
      C_LOAD(1)
{
    m_ui->setupUi(this);
    QHeaderView *header = m_ui->categoryWidget->header();
    header->setResizeMode(0, QHeaderView::ResizeToContents);
    header->setResizeMode(2, QHeaderView::ResizeToContents);

    m_okIcon = QIcon(QLatin1String(":/extensionsystem/images/ok.png"));
    m_errorIcon = QIcon(QLatin1String(":/extensionsystem/images/error.png"));
    m_notLoadedIcon = QIcon(QLatin1String(":/extensionsystem/images/notloaded.png"));

    m_ui->categoryWidget->setColumnWidth(C_LOAD, 40);

    // cannot disable these
    m_whitelist << QString("Core") << QString("Locator")
                << QString("Find") << QString("TextEditor");

    p->manager = manager;
    connect(p->manager, SIGNAL(pluginsChanged()), this, SLOT(updateList()));
    connect(m_ui->categoryWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(selectPlugin(QTreeWidgetItem*)));
    connect(m_ui->categoryWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
            this, SLOT(activatePlugin(QTreeWidgetItem*)));

    updateList();
}

/*!
    \fn PluginView::~PluginView()
    \internal
*/
PluginView::~PluginView()
{
    delete p;
    delete m_ui;
}

/*!
    \fn PluginSpec *PluginView::currentPlugin() const
    Returns the current selection in the list of plugins.
*/
PluginSpec *PluginView::currentPlugin() const
{
    if (!m_ui->categoryWidget->currentItem())
        return 0;
    if (!m_ui->categoryWidget->currentItem()->data(0, Qt::UserRole).isNull())
        return m_ui->categoryWidget->currentItem()->data(0, Qt::UserRole).value<PluginSpec *>();
    return 0;
}

void PluginView::updateList()
{
    connect(m_ui->categoryWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(updatePluginSettings(QTreeWidgetItem*, int)));

    PluginCollection *defaultCollection = 0;
    foreach(PluginCollection *collection, p->manager->pluginCollections()) {
        if (collection->name().isEmpty()) {
            defaultCollection = collection;
            continue;
        }
        // State, name, load, version, vendor.
        QTreeWidgetItem *collectionItem = new QTreeWidgetItem(QStringList()
            << collection->name()
            << QString()    // state
            << QString()    // load
            << QString()    // version
            << QString());  // vendor
        m_items.append(collectionItem);

        Qt::CheckState groupState = Qt::Unchecked;
        int state = parsePluginSpecs(collectionItem, groupState, collection->plugins());

        collectionItem->setIcon(0, iconForState(state));
        collectionItem->setData(C_LOAD, Qt::CheckStateRole, QVariant(groupState));
        collectionItem->setToolTip(C_LOAD, tr("Load on Startup"));
        collectionItem->setData(0, Qt::UserRole, qVariantFromValue(collection));
    }

    // add all non-categorized plugins into utilities. could also be added as root items
    // but that makes the tree ugly.
    QTreeWidgetItem *defaultCollectionItem = new QTreeWidgetItem(QStringList()
        << QString(tr("Utilities"))
        << QString()
        << QString()
        << QString()
        << QString());

    m_items.append(defaultCollectionItem);
    Qt::CheckState groupState = Qt::Unchecked;
    int state = parsePluginSpecs(defaultCollectionItem, groupState, defaultCollection->plugins());

    defaultCollectionItem->setIcon(0, iconForState(state));
    defaultCollectionItem->setData(C_LOAD, Qt::CheckStateRole, QVariant(groupState));
    defaultCollectionItem->setToolTip(C_LOAD, tr("Load on Startup"));
    defaultCollectionItem->setData(0, Qt::UserRole, qVariantFromValue(defaultCollection));

    foreach (PluginSpec *spec, m_specToItem.keys())
        toggleRelatedPlugins(spec, spec->isEnabled() && !spec->isDisabledByDependency());

    m_ui->categoryWidget->clear();
    if (!m_items.isEmpty()) {
        m_ui->categoryWidget->addTopLevelItems(m_items);
        m_ui->categoryWidget->expandAll();
    }

    m_ui->categoryWidget->sortItems(0, Qt::AscendingOrder);
    if (m_ui->categoryWidget->topLevelItemCount())
        m_ui->categoryWidget->setCurrentItem(m_ui->categoryWidget->topLevelItem(0));
}

int PluginView::parsePluginSpecs(QTreeWidgetItem *parentItem, Qt::CheckState &groupState, QList<PluginSpec*> plugins)
{
    int ret = 0;
    int loadCount = 0;

    for (int i = 0; i < plugins.length(); ++i) {
        PluginSpec *spec = plugins[i];
        if (spec->hasError())
            ret |= ParsedWithErrors;

        QTreeWidgetItem *pluginItem = new QTreeWidgetItem(QStringList()
            << spec->name()
            << QString()    // load on startup
            << QString::fromLatin1("%1 (%2)").arg(spec->version(), spec->compatVersion())
            << spec->vendor());

        pluginItem->setToolTip(0, QDir::toNativeSeparators(spec->filePath()));
        bool ok = !spec->hasError();
        QIcon icon = ok ? m_okIcon : m_errorIcon;
        if (ok && (spec->state() != PluginSpec::Running))
            icon = m_notLoadedIcon;

        pluginItem->setIcon(0, icon);
        pluginItem->setData(0, Qt::UserRole, qVariantFromValue(spec));

        Qt::CheckState state = Qt::Unchecked;
        if (spec->isEnabled()) {
            state = Qt::Checked;
            ++loadCount;
        }

        if (!m_whitelist.contains(spec->name()))
            pluginItem->setData(C_LOAD, Qt::CheckStateRole, state);
        else {
            QColor disabledColor = palette().color(QPalette::Disabled,QPalette::WindowText).lighter(120);
            pluginItem->setData(C_LOAD, Qt::CheckStateRole, Qt::Checked);
            pluginItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            pluginItem->setSizeHint(C_LOAD, QSize(1,1));
            pluginItem->setForeground(C_LOAD, QBrush(disabledColor)); // QBrush(Qt::white, Qt::NoBrush));
            //pluginItem->setBackground(C_LOAD, QBrush(Qt::white, Qt::NoBrush));
        }
        pluginItem->setToolTip(C_LOAD, tr("Load on Startup"));

        m_specToItem.insert(spec, pluginItem);

        if (parentItem)
            parentItem->addChild(pluginItem);
        else
            m_items.append(pluginItem);

    }

    if (loadCount == plugins.length()) {
        groupState = Qt::Checked;
        ret |= ParsedAll;
    } else if (loadCount == 0) {
        groupState = Qt::Unchecked;
        ret |= ParsedNone;
    } else {
        groupState = Qt::PartiallyChecked;
        ret = ret | ParsedPartial;
    }
    return ret;
}

QIcon PluginView::iconForState(int state)
{
    if (state & ParsedWithErrors)
        return m_errorIcon;

    if (state & ParsedNone || state & ParsedPartial)
        return m_notLoadedIcon;

    return m_okIcon;
}

void PluginView::selectPlugin(QTreeWidgetItem *current)
{
    if (!current)
        emit currentPluginChanged(0);
    else if (current->data(0, Qt::UserRole).canConvert<PluginSpec*>())
        emit currentPluginChanged(current->data(0, Qt::UserRole).value<PluginSpec *>());
    else
        emit currentPluginChanged(0);

}

void PluginView::activatePlugin(QTreeWidgetItem *item)
{
    if (item->data(0, Qt::UserRole).canConvert<PluginSpec*>()) {
        emit pluginActivated(item->data(0, Qt::UserRole).value<PluginSpec *>());
    } else
        emit pluginActivated(0);
}

void PluginView::updatePluginSettings(QTreeWidgetItem *item, int column)
{
    if (!m_allowCheckStateUpdate)
        return;

    m_allowCheckStateUpdate = false;

    bool loadOnStartup = item->data(C_LOAD, Qt::CheckStateRole).toBool();

    if (item->data(0, Qt::UserRole).canConvert<PluginSpec*>()) {
        PluginSpec *spec = item->data(0, Qt::UserRole).value<PluginSpec *>();

        if (column == C_LOAD) {

            spec->setEnabled(loadOnStartup);
            toggleRelatedPlugins(spec, loadOnStartup);

            if (item->parent()) {
                PluginCollection *collection = item->parent()->data(0, Qt::UserRole).value<PluginCollection *>();
                Qt::CheckState state = Qt::PartiallyChecked;
                int loadCount = 0;
                for (int i = 0; i < collection->plugins().length(); ++i) {
                    if (collection->plugins().at(i)->isEnabled())
                        ++loadCount;
                }
                if (loadCount == collection->plugins().length())
                    state = Qt::Checked;
                else if (loadCount == 0)
                    state = Qt::Unchecked;

                item->parent()->setData(C_LOAD, Qt::CheckStateRole, state);
            }

            emit pluginSettingsChanged(spec);
        }

    } else {
        PluginCollection *collection = item->data(0, Qt::UserRole).value<PluginCollection *>();
        for (int i = 0; i < collection->plugins().length(); ++i) {
            PluginSpec *spec = collection->plugins().at(i);
            QTreeWidgetItem *child = m_specToItem.value(spec);

            if (!m_whitelist.contains(spec->name())) {
                spec->setEnabled(loadOnStartup);
                Qt::CheckState state = (loadOnStartup ? Qt::Checked : Qt::Unchecked);
                child->setData(C_LOAD, Qt::CheckStateRole, state);
                toggleRelatedPlugins(spec, loadOnStartup);
            } else {
                child->setData(C_LOAD, Qt::CheckStateRole, Qt::Checked);
                child->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
        }
        emit pluginSettingsChanged(collection->plugins().first());
    }

    m_allowCheckStateUpdate = true;
}

void PluginView::toggleRelatedPlugins(PluginSpec *modifiedPlugin, bool isPluginEnabled)
{

    for(int i = 0; i < modifiedPlugin->providesSpecs().length(); ++i) {
        PluginSpec *spec = modifiedPlugin->providesSpecs().at(i);
        QTreeWidgetItem *childItem = m_specToItem.value(spec);

        if (childItem->isDisabled() != !isPluginEnabled) {
            childItem->setDisabled(!isPluginEnabled);
            if (childItem->parent() && !childItem->parent()->isExpanded())
                childItem->parent()->setExpanded(true);


            toggleRelatedPlugins(spec, isPluginEnabled);
        }
    }
}

