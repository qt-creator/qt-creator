// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimcodestylesettingspage.h"

#include "../editor/nimeditorfactory.h"
#include "../editor/nimindenter.h"
#include "../nimconstants.h"
#include "../nimtr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <texteditor/codestyleeditor.h>
#include <texteditor/codestylepool.h>
#include <texteditor/displaysettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/indenter.h>
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/snippets/snippeteditor.h>
#include <texteditor/tabsettings.h>
#include <texteditor/tabsettingswidget.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>

#include <utils/id.h>
#include <utils/layoutbuilder.h>
#include <utils/shutdownguard.h>

#include <QTextDocument>
#include <QVBoxLayout>

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

    Id languageId() final
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

static SimpleCodeStylePreferences *m_globalCodeStyle = nullptr;
static CodeStylePool *pool = nullptr;

SimpleCodeStylePreferences *globalCodeStyle()
{
    QTC_CHECK(m_globalCodeStyle);
    return m_globalCodeStyle;
}

static void createGlobalCodeStyle()
{
    auto factory = new NimCodeStylePreferencesFactory;
    TextEditorSettings::registerCodeStyleFactory(factory);

    // code style pool
    pool = new CodeStylePool(factory);
    TextEditorSettings::registerCodeStylePool(Nim::Constants::C_NIMLANGUAGE_ID, pool);

    m_globalCodeStyle = new SimpleCodeStylePreferences();
    m_globalCodeStyle->setDelegatingPool(pool);
    m_globalCodeStyle->setDisplayName(Tr::tr("Global", "Settings"));
    m_globalCodeStyle->setId(Nim::Constants::C_NIMGLOBALCODESTYLE_ID);
    pool->addCodeStyle(m_globalCodeStyle);
    TextEditorSettings::registerCodeStyle(Nim::Constants::C_NIMLANGUAGE_ID, m_globalCodeStyle);

    auto nimCodeStyle = new SimpleCodeStylePreferences();
    nimCodeStyle->setId("nim");
    nimCodeStyle->setDisplayName(Tr::tr("Nim"));
    nimCodeStyle->setReadOnly(true);

    TabSettings nimTabSettings;
    nimTabSettings.m_tabPolicy = TabSettings::SpacesOnlyTabPolicy;
    nimTabSettings.m_tabSize = 2;
    nimTabSettings.m_indentSize = 2;
    nimTabSettings.m_continuationAlignBehavior = TabSettings::ContinuationAlignWithIndent;
    nimCodeStyle->setTabSettings(nimTabSettings);

    pool->addCodeStyle(nimCodeStyle);

    m_globalCodeStyle->setCurrentDelegate(nimCodeStyle);

    pool->loadCustomCodeStyles();

    // load global settings (after built-in settings are added to the pool)
    m_globalCodeStyle->fromSettings(Nim::Constants::C_NIMLANGUAGE_ID);

    TextEditorSettings::registerMimeTypeForLanguageId(Nim::Constants::C_NIM_MIMETYPE,
                                                      Nim::Constants::C_NIMLANGUAGE_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(Nim::Constants::C_NIM_SCRIPT_MIMETYPE,
                                                      Nim::Constants::C_NIMLANGUAGE_ID);
}

static void destroyGlobalCodeStyle()
{
    TextEditorSettings::unregisterCodeStyle(Nim::Constants::C_NIMLANGUAGE_ID);
    TextEditorSettings::unregisterCodeStylePool(Nim::Constants::C_NIMLANGUAGE_ID);
    TextEditorSettings::unregisterCodeStyleFactory(Nim::Constants::C_NIMLANGUAGE_ID);

    delete m_globalCodeStyle;
    m_globalCodeStyle = nullptr;

    delete pool;
    pool = nullptr;
}

class NimCodeStyleSettingsWidget : public Core::IOptionsPageWidget
{
public:
    NimCodeStyleSettingsWidget()
    {
        auto originalTabPreferences = globalCodeStyle();
        m_nimCodeStylePreferences = new SimpleCodeStylePreferences(this);
        m_nimCodeStylePreferences->setDelegatingPool(originalTabPreferences->delegatingPool());
        m_nimCodeStylePreferences->setTabSettings(originalTabPreferences->tabSettings());
        m_nimCodeStylePreferences->setCurrentDelegate(originalTabPreferences->currentDelegate());
        m_nimCodeStylePreferences->setId(originalTabPreferences->id());

        auto factory = TextEditorSettings::codeStyleFactory(Nim::Constants::C_NIMLANGUAGE_ID);
        CodeStyleEditorWidget *editor
            = factory->createCodeStyleEditor({}, m_nimCodeStylePreferences);

        auto layout = new QVBoxLayout(this);
        layout->addWidget(editor);
    }

    void apply() final
    {
        QTC_ASSERT(m_globalCodeStyle, return);
        m_globalCodeStyle->toSettings(Nim::Constants::C_NIMLANGUAGE_ID);
    }

private:
    TextEditor::SimpleCodeStylePreferences *m_nimCodeStylePreferences;
};

// NimCodeStyleSettingsPage

class NimCodeStyleSettingsPage final : public Core::IOptionsPage
{
public:
    NimCodeStyleSettingsPage()
    {
        setId(Nim::Constants::C_NIMCODESTYLESETTINGSPAGE_ID);
        setDisplayName(Tr::tr("Code Style"));
        setCategory(Nim::Constants::C_NIMCODESTYLESETTINGSPAGE_CATEGORY);
        setWidgetCreator([] { return new NimCodeStyleSettingsWidget; });

        createGlobalCodeStyle();
    }

    ~NimCodeStyleSettingsPage() { destroyGlobalCodeStyle(); }
};

void Internal::setupNimCodeStyle()
{
    static GuardedObject<NimCodeStyleSettingsPage> theNimCodeStyleSettingsPage;
}

} // Nim
