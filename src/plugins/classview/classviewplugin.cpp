// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "classviewmanager.h"
#include "classviewnavigationwidgetfactory.h"

#include <extensionsystem/iplugin.h>

namespace ClassView::Internal {

///////////////////////////////// Plugin //////////////////////////////////

/*!
    \class ClassViewPlugin
    \brief The ClassViewPlugin class implements the Class View plugin.

    The Class View shows the namespace and class hierarchy of the currently open
    projects in the sidebar.
*/

class ClassViewPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ClassView.json")

    void initialize() final
    {
        setupClassViewNavigationWidgetFactory();
        setupClassViewManager(this);
    }
};

} // ClassView::Internal

#include "classviewplugin.moc"
