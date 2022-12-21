// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mytypes.h"

namespace ScxmlEditor {

namespace PluginInterface {

class ScxmlUiFactory;
class ScxmlDocument;

/**
 * @brief The ISCEditor interface for the all SCEditor plugins
 *
 * Every SCEditor plugin must implement this interface. SCEditor application will load all plugins which have listed
 * in the environment variable QT_SCEDITOR_PLUGINS and which implements this interface. The function init() will be called
 * immediately after plugin has successfully loaded. See the @ScxmlUiFactory for more information how to register editor etc for the UI.
 * The function refresh() will be called when it is time to refresh data. Plugin itself can decide if this is necessary or not.
 * Normally this function will be called when application get focus.
 */
class ISCEditor
{
public:
    /**
     * @brief init - pure virtual function to init and forward the pointer of the ScxmlUiFactory to the plugin.
     * @param factory - pointer to the ScxmlUiFactory
     */
    virtual void init(ScxmlUiFactory *factory) = 0;

    /**
     * @brief documentChanged - when document will changed this function will be called
     * @param type - change reason
     * @param doc - pointer to ScxmlDocument
     */
    virtual void documentChanged(DocumentChangeType type, ScxmlDocument *doc) = 0;

    /**
     * @brief refresh - tell the plugin that it is time to update all data if necessary
     */
    virtual void refresh() = 0;

    /**
     * @brief detach - here should be unregister all objects
     */
    virtual void detach() = 0;
};

} // namespace PluginInterface
} // namespace ScxmlEditor

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(ScxmlEditor::PluginInterface::ISCEditor, "StateChartEditor.ISCEditor/1.0")
QT_END_NAMESPACE
