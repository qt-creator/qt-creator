// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimcodestylepreferencesfactory.h"

#include "nim/editor/nimeditorfactory.h"
#include "nim/editor/nimindenter.h"
#include "nim/nimconstants.h"
#include "nim/nimtr.h"

#include <texteditor/codestyleeditor.h>
#include <texteditor/displaysettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/indenter.h>
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/snippets/snippeteditor.h>
#include <texteditor/tabsettingswidget.h>
#include <texteditor/tabsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>

#include <utils/id.h>
#include <utils/layoutbuilder.h>

#include <QTextDocument>

using namespace TextEditor;
using namespace Utils;

namespace Nim {

class NimCodeStylePreferencesWidget : public CodeStyleEditorWidget
{
public:
    NimCodeStylePreferencesWidget(ICodeStylePreferences *preferences, QWidget *parent = nullptr);

private:
    void decorateEditor(const FontSettings &fontSettings);
    void setVisualizeWhitespace(bool on);
    void updatePreview();

    ICodeStylePreferences *m_preferences;
    SnippetEditorWidget *m_previewTextEdit;
};

NimCodeStylePreferencesWidget::NimCodeStylePreferencesWidget(
    ICodeStylePreferences *preferences, QWidget *parent)
    : CodeStyleEditorWidget(parent)
    , m_preferences(preferences)
{
    auto tabSettingsWidget = new TabSettingsWidget;
    tabSettingsWidget->setPreferences(preferences);

    m_previewTextEdit = new SnippetEditorWidget;
    m_previewTextEdit->setPlainText(Nim::Constants::C_NIMCODESTYLEPREVIEWSNIPPET);

    using namespace Layouting;
    Row {
        Column {
            tabSettingsWidget,
            st,
        },
        m_previewTextEdit,
        noMargin,
    }.attachTo(this);

    decorateEditor(TextEditorSettings::fontSettings());
    connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
            this, &NimCodeStylePreferencesWidget::decorateEditor);

    connect(m_preferences, &ICodeStylePreferences::currentTabSettingsChanged,
            this, &NimCodeStylePreferencesWidget::updatePreview);

    setVisualizeWhitespace(true);

    updatePreview();
}

void NimCodeStylePreferencesWidget::decorateEditor(const FontSettings &fontSettings)
{
    m_previewTextEdit->textDocument()->setFontSettings(fontSettings);
    NimEditorFactory::decorateEditor(m_previewTextEdit);
}

void NimCodeStylePreferencesWidget::setVisualizeWhitespace(bool on)
{
    DisplaySettingsData displaySettings = m_previewTextEdit->displaySettings();
    displaySettings.m_visualizeWhitespace = on;
    m_previewTextEdit->setDisplaySettings(displaySettings);
}

void NimCodeStylePreferencesWidget::updatePreview()
{
    QTextDocument *doc = m_previewTextEdit->document();

    const TabSettings &ts = m_preferences
            ? m_preferences->currentTabSettings()
            : TextEditorSettings::codeStyle()->tabSettings();
    m_previewTextEdit->textDocument()->setTabSettings(ts);

    QTextBlock block = doc->firstBlock();
    QTextCursor tc = m_previewTextEdit->textCursor();
    tc.beginEditBlock();
    while (block.isValid()) {
        m_previewTextEdit->textDocument()->indenter()->indentBlock(block, QChar::Null, ts);
        block = block.next();
    }
    tc.endEditBlock();
}

// NimCodeStyleEditor

class NimCodeStyleEditor final : public CodeStyleEditor
{
public:
    static NimCodeStyleEditor *create(
        const ICodeStylePreferencesFactory *factory,
        const FilePath &projectFile,
        ICodeStylePreferences *codeStyle,
        QWidget *parent)
    {
        auto editor = new NimCodeStyleEditor{parent};
        editor->init(factory, projectFile, codeStyle);
        return editor;
    }

private:
    NimCodeStyleEditor(QWidget *parent)
        : CodeStyleEditor{parent}
    {}

    CodeStyleEditorWidget *createEditorWidget(
        const FilePath & /*projectFile*/,
        ICodeStylePreferences *codeStyle,
        QWidget *parent) const final
    {
        return new NimCodeStylePreferencesWidget(codeStyle, parent);
    }

    QString previewText() const final
    {
        return Constants::C_NIMCODESTYLEPREVIEWSNIPPET;
    }

    QString snippetProviderGroupId() const final
    {
        return Constants::C_NIMSNIPPETSGROUP_ID;
    }
};

// NimCodeStylePreferencesFactory

class NimCodeStylePreferencesFactory final : public ICodeStylePreferencesFactory
{
public:
    NimCodeStylePreferencesFactory() = default;

private:
    CodeStyleEditorWidget *createCodeStyleEditor(
            const FilePath &projectFile,
            ICodeStylePreferences *codeStyle,
            QWidget *parent) const final
    {
        return NimCodeStyleEditor::create(this, projectFile, codeStyle, parent);
    }

    Utils::Id languageId() final
    {
        return Constants::C_NIMLANGUAGE_ID;
    }

    QString displayName() final
    {
        return Tr::tr(Constants::C_NIMLANGUAGE_NAME);
    }

    ICodeStylePreferences *createCodeStyle() const final
    {
        return new SimpleCodeStylePreferences();
    }

    Indenter *createIndenter(QTextDocument *doc) const final
    {
        return createNimIndenter(doc);
    }
};

ICodeStylePreferencesFactory *createNimCodeStylePreferencesFactory()
{
    return new NimCodeStylePreferencesFactory;
}

} // Nim
