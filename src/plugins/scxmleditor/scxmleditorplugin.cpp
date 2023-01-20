// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scxmleditorplugin.h"

#include "scxmleditorfactory.h"

#include <coreplugin/designmode.h>

using namespace Core;

namespace ScxmlEditor {
namespace Internal {

class ScxmlEditorPluginPrivate
{
public:
    ScxmlEditorFactory editorFactory;
};

ScxmlEditorPlugin::~ScxmlEditorPlugin()
{
    delete d;
}

void ScxmlEditorPlugin::initialize()
{
    d = new ScxmlEditorPluginPrivate;
}

void ScxmlEditorPlugin::extensionsInitialized()
{
    DesignMode::setDesignModeIsRequired();
}

} // Internal
} // ScxmlEditor
