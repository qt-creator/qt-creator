// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "designerconstants.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QDesignerFormEditorInterface;
QT_END_NAMESPACE

namespace Core {
class IEditor;
class IOptionsPage;
}

namespace SharedTools { class WidgetHost; }

namespace Designer {

class FormWindowEditor;

namespace Internal {

/** FormEditorW is a singleton that stores the Designer CoreInterface and
  * performs centralized operations. The instance() function will return an
  * instance. However, it must be manually deleted when unloading the
  * plugin. Since fully initializing Designer at startup is expensive, the
  * class has an internal partial initialisation stage "RegisterPlugins"
  * which is there to register the Creator plugin objects
  * that must be present at startup (settings pages, actions).
  * The plugin uses this stage at first by calling ensureInitStage().
  * Requesting an editor via instance() will fully initialize the class.
  * This is based on the assumption that the Designer settings work with
  * no plugins loaded.
  *
  * The form editor shows a read-only XML editor in edit mode and Qt Designer
  * in Design mode. */
class FormEditorW : public QObject
{
public:
    enum InitializationStage {
        // Register Creator plugins (settings pages, actions)
        RegisterPlugins,
        // Subwindows of the designer are initialized
        SubwindowsInitialized,
        // Fully initialized for handling editor requests
        FullyInitialized
    };

    // Create an instance and initialize up to stage s
    static void ensureInitStage(InitializationStage s);
    // Deletes an existing instance if there is one.
    static void deleteInstance();

    static Core::IEditor *createEditor();

    static QDesignerFormEditorInterface *designerEditor();
    static QWidget * const *designerSubWindows();

    static SharedTools::WidgetHost *activeWidgetHost();
    static FormWindowEditor *activeEditor();
    static QList<Core::IOptionsPage *> optionsPages();
};

} // namespace Internal
} // namespace Designer
