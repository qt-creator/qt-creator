// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/ieditorfactory.h>

namespace Designer {
namespace Internal {

class FormEditorFactory final : public Core::IEditorFactory
{
public:
    FormEditorFactory();
};

} // namespace Internal
} // namespace Designer
