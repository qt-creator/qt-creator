/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PLUGINVIEW_H
#define PLUGINVIEW_H

#include "extensionsystem_global.h"

#include <QHash>
#include <QWidget>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace ExtensionSystem {

class PluginManager;
class PluginSpec;
class PluginCollection;

namespace Internal {
namespace Ui {
    class PluginView;
} // namespace Ui
} // namespace Internal

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

private slots:
    void updatePluginSettings(QTreeWidgetItem *item, int column);
    void updateList();
    void selectPlugin(QTreeWidgetItem *current);
    void activatePlugin(QTreeWidgetItem *item);

private:
    enum ParsedState { ParsedNone = 1, ParsedPartial = 2, ParsedAll = 4, ParsedWithErrors = 8};
    QIcon iconForState(int state);
    void updatePluginDependencies();
    int parsePluginSpecs(QTreeWidgetItem *parentItem, Qt::CheckState &groupState, QList<PluginSpec*> plugins);

    Internal::Ui::PluginView *m_ui;
    QList<QTreeWidgetItem*> m_items;
    QHash<PluginSpec*, QTreeWidgetItem*> m_specToItem;

    QStringList m_whitelist;
    QIcon m_okIcon;
    QIcon m_errorIcon;
    QIcon m_notLoadedIcon;
    bool m_allowCheckStateUpdate;

    const int C_LOAD;
};

} // namespae ExtensionSystem

#endif // PLUGIN_VIEW_H
