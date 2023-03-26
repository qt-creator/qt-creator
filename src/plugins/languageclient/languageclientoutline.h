// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/ioutlinewidget.h>

namespace TextEditor {
class TextDocument;
class BaseTextEditor;
} // namespace TextEditor
namespace Utils { class TreeViewComboBox; }

namespace LanguageClient {

class Client;

class LanguageClientOutlineWidgetFactory : public TextEditor::IOutlineWidgetFactory
{
public:
    using IOutlineWidgetFactory::IOutlineWidgetFactory;

    static Utils::TreeViewComboBox *createComboBox(Client *client, TextEditor::BaseTextEditor *editor);
    // IOutlineWidgetFactory interface
public:
    bool supportsEditor(Core::IEditor *editor) const override;
    TextEditor::IOutlineWidget *createWidget(Core::IEditor *editor) override;
    bool supportsSorting() const override { return true; }
};

} // namespace LanguageClient
