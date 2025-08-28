// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codestyleeditor.h"

#include "codestyleselectorwidget.h"
#include "displaysettings.h"
#include "icodestylepreferences.h"
#include "icodestylepreferencesfactory.h"
#include "indenter.h"
#include "snippets/snippeteditor.h"
#include "snippets/snippetprovider.h"
#include "textdocument.h"
#include "texteditortr.h"

#include <coreplugin/icore.h>
#include <utils/filepath.h>

#include <QChar>
#include <QFont>
#include <QLabel>
#include <QTextBlock>
#include <QVBoxLayout>

namespace TextEditor {

void CodeStyleEditor::init(
    const ICodeStylePreferencesFactory *factory,
    const ProjectWrapper &project,
    ICodeStylePreferences *codeStyle)
{
    m_selector = createCodeStyleSelectorWidget(codeStyle);
    m_layout->addWidget(m_selector);
    if (!project) {
        m_editor = createEditorWidget(project.project(), codeStyle);
        if (m_editor)
            m_layout->addWidget(m_editor);
        return;
    }

    m_preview = createPreviewWidget(factory, project, codeStyle, m_editor);
    m_layout->addWidget(m_preview);

    QLabel *label = new QLabel(
        Tr::tr("Edit preview contents to see how the current settings "
               "are applied to custom code snippets. Changes in the preview "
               "do not affect the current settings."),
        m_editor);
    QFont font = label->font();
    font.setItalic(true);
    label->setFont(font);
    label->setWordWrap(true);
    m_layout->addWidget(label);
}

CodeStyleSelectorWidget *CodeStyleEditor::createCodeStyleSelectorWidget(
    ICodeStylePreferences *codeStyle, QWidget *parent) const
{
    auto selector = new CodeStyleSelectorWidget{parent};
    selector->setCodeStyle(codeStyle);
    return selector;
}

SnippetEditorWidget *CodeStyleEditor::createPreviewWidget(
    const ICodeStylePreferencesFactory *factory,
    const ProjectWrapper &project,
    ICodeStylePreferences *codeStyle,
    QWidget *parent) const
{
    auto preview = new SnippetEditorWidget{parent};
    DisplaySettings displaySettings = preview->displaySettings();
    displaySettings.m_visualizeWhitespace = true;
    preview->setDisplaySettings(displaySettings);
    const QString groupId = snippetProviderGroupId();
    SnippetProvider::decorateEditor(preview, groupId);
    preview->setPlainText(previewText());

    Indenter *indenter = factory->createIndenter(preview->document());
    if (indenter) {
        indenter->setOverriddenPreferences(codeStyle);
        const Utils::FilePath fileName = project ? project.projectFilePath().pathAppended(
                                                       "snippet.cpp")
                                                 : Core::ICore::userResourcePath("snippet.cpp");
        indenter->setFileName(fileName);
        preview->textDocument()->setIndenter(indenter);
    } else {
        preview->textDocument()->setCodeStyle(codeStyle);
    }

    const auto updatePreview = [preview, codeStyle]() {
        QTextDocument *doc = preview->document();

        preview->textDocument()->indenter()->invalidateCache();

        QTextBlock block = doc->firstBlock();
        QTextCursor tc = preview->textCursor();
        tc.beginEditBlock();
        while (block.isValid()) {
            preview->textDocument()
                ->indenter()
                ->indentBlock(block, QChar::Null, codeStyle->currentTabSettings());
            block = block.next();
        }
        tc.endEditBlock();
    };

    connect(codeStyle, &ICodeStylePreferences::currentTabSettingsChanged, this, updatePreview);
    connect(codeStyle, &ICodeStylePreferences::currentValueChanged, this, updatePreview);
    connect(codeStyle, &ICodeStylePreferences::currentPreferencesChanged, this, updatePreview);

    updatePreview();

    return preview;
}

CodeStyleEditor::CodeStyleEditor(QWidget *parent)
    : CodeStyleEditorWidget(parent)
{
    m_layout = new QVBoxLayout{this};
    m_layout->setContentsMargins(0, 0, 0, 0);
}

void CodeStyleEditor::apply()
{
    if (m_editor != nullptr)
        m_editor->apply();
}

void CodeStyleEditor::finish()
{
    if (m_editor != nullptr)
        m_editor->finish();
}

} // TextEditor
