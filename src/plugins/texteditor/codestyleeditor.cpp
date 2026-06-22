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
#include <utils/guiutils.h>
#include <utils/infolabel.h>

#include <QChar>
#include <QFont>
#include <QLabel>
#include <QTextBlock>
#include <QVBoxLayout>

using namespace Utils;

namespace TextEditor {

CodeStyleEditor::CodeStyleEditor()
{
    m_layout = new QVBoxLayout{this};
    m_layout->setContentsMargins(0, 0, 0, 0);
}

void CodeStyleEditor::apply() {}

void CodeStyleEditor::cancel() {}

void CodeStyleEditor::addSelector(CodeStyleSelectorWidget *selector)
{
    m_layout->addWidget(selector);
    Utils::installMarkSettingsDirtyTriggerRecursively(selector);
}

QWidget *CodeStyleEditor::addInfoLabel()
{
    auto infoLabel = new InfoLabel(Tr::tr("All changes below take effect immediately."),
                                   Utils::InfoLabel::Information);
    infoLabel->setFilled(true);
    // Wrap in a plain container: InfoLabel uses its own contentsMargins to place
    // its icon, so callers indent the container instead of the label itself.
    auto container = new QWidget;
    auto layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(infoLabel);
    m_layout->addWidget(container);
    return container;
}

void CodeStyleEditor::addHeaderWidget(QWidget *widget)
{
    m_layout->insertWidget(0, widget);
}

void CodeStyleEditor::addEditorWidget(QWidget *editor)
{
    m_layout->addWidget(editor);
}

QWidget *CodeStyleEditor::addExpandingFiller()
{
    auto filler = new QWidget;
    filler->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_layout->addWidget(filler);
    return filler;
}

QWidget *CodeStyleEditor::setupPreview(SnippetEditorWidget *preview, Indenter *indenter,
                                       const FilePath &projectFile, ICodeStylePreferences *codeStyle)
{
    indenter->setOverriddenPreferences(codeStyle);
    const FilePath fileName = !projectFile.isEmpty()
        ? projectFile.pathAppended("snippet.cpp")
        : Core::ICore::userResourcePath("snippet.cpp");
    indenter->setFileName(fileName);
    preview->textDocument()->setIndenter(indenter);

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

    m_layout->addWidget(preview);

    QLabel *label = new QLabel(
        Tr::tr("Edit preview contents to see how the current settings "
               "are applied to custom code snippets. Changes in the preview "
               "do not affect the current settings."));
    QFont font = label->font();
    font.setItalic(true);
    label->setFont(font);
    label->setWordWrap(true);
    m_layout->addWidget(label);
    return label;
}

class CodeStyleProjectPreviewEditor final : public CodeStyleEditor
{
public:
    CodeStyleProjectPreviewEditor(
        const ICodeStylePreferencesFactory *factory,
        const FilePath &projectFile,
        ICodeStylePreferences *codeStyle)
        : m_selector{projectFile, this}
    {
        m_selector.setCodeStyle(codeStyle);
        addSelector(&m_selector);
        addInfoLabel();

        DisplaySettingsData displaySettings = m_preview.displaySettings();
        displaySettings.m_visualizeWhitespace = true;
        m_preview.setDisplaySettings(displaySettings);
        SnippetProvider::decorateEditor(&m_preview, factory->snippetGroupId());
        m_preview.setPlainText(factory->previewText());
        setupPreview(&m_preview, factory->createIndenter(m_preview.document()), projectFile, codeStyle);
    }

private:
    CodeStyleSelectorWidget m_selector;
    SnippetEditorWidget m_preview;
};

CodeStyleEditor *ICodeStylePreferencesFactory::createProjectEditor(
    const FilePath &projectFile, ICodeStylePreferences *codeStyle) const
{
    return new CodeStyleProjectPreviewEditor{this, projectFile, codeStyle};
}

} // TextEditor
