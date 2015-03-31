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

#ifndef PLUGINVIEW_H
#define PLUGINVIEW_H

#include "extensionsystem_global.h"

#include <QWidget>
#include <QSet>
#include <QHash>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Utils {
class TreeItem;
class TreeModel;
class TreeView;
} // namespace Utils

namespace ExtensionSystem {

class PluginManager;
class PluginSpec;
class PluginItem;
class CollectionItem;

class EXTENSIONSYSTEM_EXPORT PluginView : public QWidget
{
    Q_OBJECT

public:
    explicit PluginView(QWidget *parent = 0);
    ~PluginView();

    PluginSpec *currentPlugin() const;

signals:
    void currentPluginChanged(ExtensionSystem::PluginSpec *spec);
    void pluginActivated(ExtensionSystem::PluginSpec *spec);
    void pluginSettingsChanged(ExtensionSystem::PluginSpec *spec);

private:
    PluginSpec *pluginForIndex(const QModelIndex &index) const;
    void updatePlugins();
    bool setPluginsEnabled(const QSet<PluginSpec *> &plugins, bool enable);

    Utils::TreeView *m_categoryView;
    Utils::TreeModel *m_model;
    QSortFilterProxyModel *m_sortModel;

    friend class CollectionItem;
    friend class PluginItem;
};

} // namespae ExtensionSystem

#endif // PLUGIN_VIEW_H
