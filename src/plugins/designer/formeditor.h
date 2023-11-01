// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QDesignerFormEditorInterface;
class QWidget;
QT_END_NAMESPACE

namespace Core {
class IEditor;
class IOptionsPage;
}

namespace SharedTools { class WidgetHost; }

namespace Designer {

class FormWindowEditor;

namespace Internal {

/** This is an interface to the Designer CoreInterface to
  * performs centralized operations.
  * Since fully initializing Designer at startup is expensive, the
  * setup has an internal partial initialization stage "RegisterPlugins"
  * which is there to register the Creator plugin objects
  * that must be present at startup (settings pages, actions).
  * The plugin uses this stage at first by calling ensureInitStage().
  * Requesting an editor via instance() will fully initialize the class.
  * This is based on the assumption that the Designer settings work with
  * no plugins loaded.
  *
  * The form editor shows a read-only XML editor in edit mode and Qt Designer
  * in Design mode. */

enum InitializationStage {
    // Register Creator plugins (settings pages, actions)
    RegisterPlugins,
    // Subwindows of the designer are initialized
    SubwindowsInitialized,
    // Fully initialized for handling editor requests
    FullyInitialized
};

// Create an instance and initialize up to stage s
void ensureInitStage(InitializationStage s);
// Deletes an existing instance if there is one.
void deleteInstance();

Core::IEditor *createEditor();

QDesignerFormEditorInterface *designerEditor();
QWidget * const *designerSubWindows();

SharedTools::WidgetHost *activeWidgetHost();
FormWindowEditor *activeEditor();
QList<Core::IOptionsPage *> optionsPages();

void setQtPluginPath(const QString &qtPluginPath);
void addPluginPath(const QString &pluginPath);

} // namespace Internal
} // namespace Designer
