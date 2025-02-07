// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimcodestylepreferencesfactory.h"

#include "nim/editor/nimindenter.h"
#include "nim/nimconstants.h"
#include "nim/nimtr.h"
#include "nimcodestylepreferenceswidget.h"

#include <texteditor/codestyleeditor.h>
#include <texteditor/indenter.h>
#include <texteditor/simplecodestylepreferences.h>
#include <utils/id.h>

#include <QString>
#include <QTextDocument>
#include <QWidget>

using namespace TextEditor;

namespace Nim {

class NimCodeStyleEditor final : public TextEditor::CodeStyleEditor
{
public:
    static NimCodeStyleEditor *create(
        const TextEditor::ICodeStylePreferencesFactory *factory,
        const ProjectWrapper &project,
        TextEditor::ICodeStylePreferences *codeStyle,
        QWidget *parent = nullptr);

private:
    NimCodeStyleEditor(QWidget *parent = nullptr);

    CodeStyleEditorWidget *createEditorWidget(
        const void * /*project*/,
        TextEditor::ICodeStylePreferences *codeStyle,
        QWidget *parent = nullptr) const override;
    QString previewText() const override;
    QString snippetProviderGroupId() const override;
};

NimCodeStyleEditor *NimCodeStyleEditor::create(
    const ICodeStylePreferencesFactory *factory,
    const ProjectWrapper &project,
    ICodeStylePreferences *codeStyle,
    QWidget *parent)
{
    auto editor = new NimCodeStyleEditor{parent};
    editor->init(factory, project, codeStyle);
    return editor;
}

NimCodeStyleEditor::NimCodeStyleEditor(QWidget *parent)
    : CodeStyleEditor{parent}
{}

CodeStyleEditorWidget *NimCodeStyleEditor::createEditorWidget(
    const void * /*project*/,
    ICodeStylePreferences *codeStyle,
    QWidget *parent) const
{
    return new NimCodeStylePreferencesWidget(codeStyle, parent);
}

QString NimCodeStyleEditor::previewText() const
{
    return Constants::C_NIMCODESTYLEPREVIEWSNIPPET;
}

QString NimCodeStyleEditor::snippetProviderGroupId() const
{
    return Constants::C_NIMSNIPPETSGROUP_ID;
}

TextEditor::CodeStyleEditorWidget *NimCodeStylePreferencesFactory::createCodeStyleEditor(
    const ProjectWrapper &project,
    TextEditor::ICodeStylePreferences *codeStyle,
    QWidget *parent) const
{
    return NimCodeStyleEditor::create(this, project, codeStyle, parent);
}

Utils::Id NimCodeStylePreferencesFactory::languageId()
{
    return Constants::C_NIMLANGUAGE_ID;
}

QString NimCodeStylePreferencesFactory::displayName()
{
    return Tr::tr(Constants::C_NIMLANGUAGE_NAME);
}

ICodeStylePreferences *NimCodeStylePreferencesFactory::createCodeStyle() const
{
    return new SimpleCodeStylePreferences();
}

Indenter *NimCodeStylePreferencesFactory::createIndenter(QTextDocument *doc) const
{
    return createNimIndenter(doc);
}

} // Nim
