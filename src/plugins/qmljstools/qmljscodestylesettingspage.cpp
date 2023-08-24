// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscodestylesettingspage.h"

#include "qmljscodestylepreferences.h"
#include "qmljscodestylepreferenceswidget.h"
#include "qmljsqtstylecodeformatter.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolssettings.h"
#include "qmljstoolstr.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <texteditor/codestyleeditor.h>
#include <texteditor/displaysettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/simplecodestylepreferenceswidget.h>
#include <texteditor/snippets/snippeteditor.h>
#include <texteditor/snippets/snippetprovider.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>
#include <utils/layoutbuilder.h>

#include <QTextStream>
#include <QVBoxLayout>

using namespace TextEditor;

namespace QmlJSTools::Internal {

// QmlJSCodeStylePreferencesWidget

QmlJSCodeStylePreferencesWidget::QmlJSCodeStylePreferencesWidget(
        const TextEditor::ICodeStylePreferencesFactory *factory, QWidget *parent)
    : TextEditor::CodeStyleEditorWidget(parent)
{
    m_tabPreferencesWidget = new SimpleCodeStylePreferencesWidget;
    m_codeStylePreferencesWidget = new QmlJSTools::QmlJSCodeStylePreferencesWidget;
    m_previewTextEdit = new SnippetEditorWidget;
    m_previewTextEdit->setPlainText(factory->previewText());
    QSizePolicy sp(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    sp.setHorizontalStretch(1);
    m_previewTextEdit->setSizePolicy(sp);

    decorateEditor(TextEditorSettings::fontSettings());

    using namespace Layouting;
    Row {
        Column {
            m_tabPreferencesWidget,
            m_codeStylePreferencesWidget,
            st,
        },
        m_previewTextEdit,
        noMargin
    }.attachTo(this);

    connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
       this, &QmlJSCodeStylePreferencesWidget::decorateEditor);

    setVisualizeWhitespace(true);

    updatePreview();
}

void QmlJSCodeStylePreferencesWidget::setPreferences(QmlJSCodeStylePreferences *preferences)
{
    m_preferences = preferences;
    m_tabPreferencesWidget->setPreferences(preferences);
    m_codeStylePreferencesWidget->setPreferences(preferences);
    if (m_preferences)
    {
        connect(m_preferences, &ICodeStylePreferences::currentTabSettingsChanged,
                this, &QmlJSCodeStylePreferencesWidget::slotSettingsChanged);
        connect(m_preferences, &QmlJSCodeStylePreferences::currentCodeStyleSettingsChanged,
                this, &QmlJSCodeStylePreferencesWidget::slotSettingsChanged);
    }
    updatePreview();
}

void QmlJSCodeStylePreferencesWidget::decorateEditor(const FontSettings &fontSettings)
{
    m_previewTextEdit->textDocument()->setFontSettings(fontSettings);
    SnippetProvider::decorateEditor(m_previewTextEdit,
                                    QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID);
}

void QmlJSCodeStylePreferencesWidget::setVisualizeWhitespace(bool on)
{
    DisplaySettings displaySettings = m_previewTextEdit->displaySettings();
    displaySettings.m_visualizeWhitespace = on;
    m_previewTextEdit->setDisplaySettings(displaySettings);
}

void QmlJSCodeStylePreferencesWidget::slotSettingsChanged()
{
    updatePreview();
}

void QmlJSCodeStylePreferencesWidget::updatePreview()
{
    QTextDocument *doc = m_previewTextEdit->document();

    const TabSettings &ts = m_preferences
            ? m_preferences->currentTabSettings()
            : TextEditorSettings::codeStyle()->tabSettings();
    m_previewTextEdit->textDocument()->setTabSettings(ts);
    CreatorCodeFormatter formatter(ts);
    formatter.invalidateCache(doc);

    QTextBlock block = doc->firstBlock();
    QTextCursor tc = m_previewTextEdit->textCursor();
    tc.beginEditBlock();
    while (block.isValid()) {
        m_previewTextEdit->textDocument()->indenter()->indentBlock(block, QChar::Null, ts);
        block = block.next();
    }
    tc.endEditBlock();
}

// QmlJSCodeStyleSettingsPageWidget

class QmlJSCodeStyleSettingsPageWidget : public Core::IOptionsPageWidget
{
public:
    QmlJSCodeStyleSettingsPageWidget()
    {

        QmlJSCodeStylePreferences *originalPreferences
                = QmlJSToolsSettings::globalCodeStyle();
        m_preferences.setDelegatingPool(originalPreferences->delegatingPool());
        m_preferences.setCodeStyleSettings(originalPreferences->codeStyleSettings());
        m_preferences.setTabSettings(originalPreferences->tabSettings());
        m_preferences.setCurrentDelegate(originalPreferences->currentDelegate());
        m_preferences.setId(originalPreferences->id());

        auto vbox = new QVBoxLayout(this);
        vbox->addWidget(new CodeStyleEditor(
            TextEditorSettings::codeStyleFactory(QmlJSTools::Constants::QML_JS_SETTINGS_ID),
            &m_preferences));
    }

    void apply() final
    {
        QmlJSCodeStylePreferences *originalPreferences = QmlJSToolsSettings::globalCodeStyle();
        if (originalPreferences->codeStyleSettings() != m_preferences.codeStyleSettings()) {
            originalPreferences->setCodeStyleSettings(m_preferences.codeStyleSettings());
            originalPreferences->toSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
        }
        if (originalPreferences->tabSettings() != m_preferences.tabSettings()) {
            originalPreferences->setTabSettings(m_preferences.tabSettings());
            originalPreferences->toSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
        }
        if (originalPreferences->currentDelegate() != m_preferences.currentDelegate()) {
            originalPreferences->setCurrentDelegate(m_preferences.currentDelegate());
            originalPreferences->toSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
        }
    }

      QmlJSCodeStylePreferences m_preferences;
};

// QmlJSCodeStyleSettingsPage

QmlJSCodeStyleSettingsPage::QmlJSCodeStyleSettingsPage()
{
    setId(Constants::QML_JS_CODE_STYLE_SETTINGS_ID);
    setDisplayName(Tr::tr(Constants::QML_JS_CODE_STYLE_SETTINGS_NAME));
    setCategory(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
    setDisplayCategory(Tr::tr("Qt Quick"));
    setCategoryIconPath(":/qmljstools/images/settingscategory_qml.png");
    setWidgetCreator([] { return new QmlJSCodeStyleSettingsPageWidget; });
}

} // QmlJSTools::Internal
