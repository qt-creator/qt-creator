// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/ieditorfactory.h>

namespace Squish::Internal {

class ObjectsMapEditorFactory : public Core::IEditorFactory
{
public:
    ObjectsMapEditorFactory();
};

} // Squish::Internal
