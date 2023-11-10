// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/ioutlinewidget.h>

namespace CppEditor::Internal {

class CppOutlineWidgetFactory final : public TextEditor::IOutlineWidgetFactory
{
public:
    bool supportsEditor(Core::IEditor *editor) const final;
    bool supportsSorting() const final { return true; }
    TextEditor::IOutlineWidget *createWidget(Core::IEditor *editor) final;
};

} // namespace CppEditor::Internal
