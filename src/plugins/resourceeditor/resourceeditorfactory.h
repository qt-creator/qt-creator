// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/ieditorfactory.h>

namespace ResourceEditor::Internal {

class ResourceEditorPlugin;

class ResourceEditorFactory final : public Core::IEditorFactory
{
public:
    explicit ResourceEditorFactory(ResourceEditorPlugin *plugin);
};

} // ResourceEditor::Internal
