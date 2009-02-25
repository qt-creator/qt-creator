/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "pluginview.h"
#include "pluginview_p.h"
#include "pluginmanager.h"
#include "pluginspec.h"
#include "ui_pluginview.h"

#include <QtGui/QHeaderView>
#include <QtGui/QTreeWidgetItem>
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

/*!
    \fn PluginView::PluginView(PluginManager *manager, QWidget *parent)
    Constructs a PluginView that gets the list of plugins from the
    given plugin \a manager with a given \a parent widget.
*/
PluginView::PluginView(PluginManager *manager, QWidget *parent)
    : QWidget(parent), 
      m_ui(new Internal::Ui::PluginView),
      p(new Internal::PluginViewPrivate)
{
    m_ui->setupUi(this);
    QHeaderView *header = m_ui->pluginWidget->header();
    header->setResizeMode(0, QHeaderView::ResizeToContents);
    header->setResizeMode(1, QHeaderView::ResizeToContents);
    header->setResizeMode(2, QHeaderView::ResizeToContents);
    m_ui->pluginWidget->sortItems(1, Qt::AscendingOrder);
    p->manager = manager;
    connect(p->manager, SIGNAL(pluginsChanged()), this, SLOT(updateList()));
    connect(m_ui->pluginWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(selectPlugin(QTreeWidgetItem*)));
    connect(m_ui->pluginWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
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
    if (!m_ui->pluginWidget->currentItem())
        return 0;
    return m_ui->pluginWidget->currentItem()->data(0, Qt::UserRole).value<PluginSpec *>();
}

void PluginView::updateList()
{
    static QIcon okIcon(":/extensionsystem/images/ok.png");
    static QIcon errorIcon(":/extensionsystem/images/error.png");
    QList<QTreeWidgetItem *> items;
    QTreeWidgetItem *currentItem = 0;
    PluginSpec *currPlugin = currentPlugin();
    foreach (PluginSpec *spec, p->manager->plugins()) {
        QTreeWidgetItem *item = new QTreeWidgetItem(QStringList()
            << ""
            << spec->name()
            << QString("%1 (%2)").arg(spec->version()).arg(spec->compatVersion())
            << spec->vendor()
            << spec->filePath());
        item->setToolTip(4, spec->filePath());
        item->setIcon(0, spec->hasError() ? errorIcon : okIcon);
        item->setData(0, Qt::UserRole, qVariantFromValue(spec));
        items.append(item);
        if (currPlugin == spec)
            currentItem = item;
    }
    m_ui->pluginWidget->clear();
    if (!items.isEmpty())
        m_ui->pluginWidget->addTopLevelItems(items);
    if (currentItem)
        m_ui->pluginWidget->setCurrentItem(currentItem);
    else if (!items.isEmpty())
        m_ui->pluginWidget->setCurrentItem(items.first());
}

void PluginView::selectPlugin(QTreeWidgetItem *current)
{
    if (!current)
        emit currentPluginChanged(0);
    else
        emit currentPluginChanged(current->data(0, Qt::UserRole).value<PluginSpec *>());
}

void PluginView::activatePlugin(QTreeWidgetItem *item)
{
    emit pluginActivated(item->data(0, Qt::UserRole).value<PluginSpec *>());
}

