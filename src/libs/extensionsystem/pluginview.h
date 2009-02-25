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

#ifndef PLUGINVIEW_H
#define PLUGINVIEW_H

#include "extensionsystem_global.h"

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace ExtensionSystem {

class PluginManager;
class PluginSpec;

namespace Internal {
    class PluginViewPrivate;
namespace Ui {
    class PluginView;
} // namespace Ui
} // namespace Internal

class EXTENSIONSYSTEM_EXPORT PluginView : public QWidget
{
    Q_OBJECT

public:
    PluginView(PluginManager *manager, QWidget *parent = 0);
    ~PluginView();

    PluginSpec *currentPlugin() const;

signals:
    void currentPluginChanged(ExtensionSystem::PluginSpec *spec);
    void pluginActivated(ExtensionSystem::PluginSpec *spec);

private slots:
    void updateList();
    void selectPlugin(QTreeWidgetItem *current);
    void activatePlugin(QTreeWidgetItem *item);

private:
    Internal::Ui::PluginView *m_ui;
    Internal::PluginViewPrivate *p;
};

} // namespae ExtensionSystem

#endif // PLUGIN_VIEW_H
