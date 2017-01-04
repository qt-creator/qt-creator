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

#pragma once

#include "extensionsystem_global.h"

#include <utils/treemodel.h>

#include <QWidget>
#include <QSet>
#include <QHash>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Utils { class TreeView; }

namespace ExtensionSystem {

class PluginManager;
class PluginSpec;

namespace Internal {
class CollectionItem;
class PluginFilterModel;
class PluginItem;
} // Internal

class EXTENSIONSYSTEM_EXPORT PluginView : public QWidget
{
    Q_OBJECT

public:
    explicit PluginView(QWidget *parent = 0);
    ~PluginView();

    PluginSpec *currentPlugin() const;
    void setFilter(const QString &filter);
    void setShowHidden(bool showHidden);
    bool isShowingHidden() const;

signals:
    void currentPluginChanged(ExtensionSystem::PluginSpec *spec);
    void pluginActivated(ExtensionSystem::PluginSpec *spec);
    void pluginSettingsChanged(ExtensionSystem::PluginSpec *spec);

private:
    PluginSpec *pluginForIndex(const QModelIndex &index) const;
    void updatePlugins();
    bool setPluginsEnabled(const QSet<PluginSpec *> &plugins, bool enable);

    Utils::TreeView *m_categoryView;
    Utils::TreeModel<Utils::TreeItem, Internal::CollectionItem, Internal::PluginItem> *m_model;
    Internal::PluginFilterModel *m_sortModel;

    friend class Internal::CollectionItem;
    friend class Internal::PluginItem;
};

} // namespae ExtensionSystem
