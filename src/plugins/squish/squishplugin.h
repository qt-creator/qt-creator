/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "squishplugin_global.h"

#include "squishsettings.h"

#include <extensionsystem/iplugin.h>

namespace Squish {
namespace Internal {

class ObjectsMapEditorFactory;
class SquishNavigationWidgetFactory;
class SquishOutputPane;
class SquishTestTreeModel;
class SquishTools;

class SquishPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Squish.json")

public:
    SquishPlugin();
    ~SquishPlugin() override;

    static SquishPlugin *instance();

    SquishSettings *squishSettings();

    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

private:
    void initializeMenuEntries();

    SquishTools * m_squishTools = nullptr;
    SquishTestTreeModel *m_treeModel = nullptr;
    SquishSettings m_squishSettings;

    SquishSettingsPage m_settingsPage{&m_squishSettings};
    SquishNavigationWidgetFactory *m_navigationWidgetFactory = nullptr;
    SquishOutputPane *m_outputPane = nullptr;
    ObjectsMapEditorFactory *m_objectsMapEditorFactory = nullptr;
};

} // namespace Internal
} // namespace Squish
