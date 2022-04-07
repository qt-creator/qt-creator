/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmljscodestylesettingspage.h"

#include "qmljscodestylepreferences.h"
#include "qmljsindenter.h"
#include "qmljsqtstylecodeformatter.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolssettings.h"
#include "ui_qmljscodestylesettingspage.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <texteditor/codestyleeditor.h>
#include <texteditor/displaysettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/snippets/snippetprovider.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>

#include <QTextStream>

using namespace TextEditor;

namespace QmlJSTools {
namespace Internal {

// ------------------ CppCodeStyleSettingsWidget

QmlJSCodeStylePreferencesWidget::QmlJSCodeStylePreferencesWidget(QWidget *parent) :
    TextEditor::CodeStyleEditorWidget(parent),
    m_ui(new Ui::QmlJSCodeStyleSettingsPage)
{
    m_ui->setupUi(this);

    decorateEditor(TextEditorSettings::fontSettings());
    connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
       this, &QmlJSCodeStylePreferencesWidget::decorateEditor);

    setVisualizeWhitespace(true);

    updatePreview();
}

QmlJSCodeStylePreferencesWidget::~QmlJSCodeStylePreferencesWidget()
{
    delete m_ui;
}

void QmlJSCodeStylePreferencesWidget::setPreferences(QmlJSCodeStylePreferences *preferences)
{
    m_preferences = preferences;
    m_ui->tabPreferencesWidget->setPreferences(preferences);
    m_ui->codeStylePreferencesWidget->setPreferences(preferences);
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
    m_ui->previewTextEdit->textDocument()->setFontSettings(fontSettings);
    SnippetProvider::decorateEditor(m_ui->previewTextEdit,
                                    QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID);
}

void QmlJSCodeStylePreferencesWidget::setVisualizeWhitespace(bool on)
{
    DisplaySettings displaySettings = m_ui->previewTextEdit->displaySettings();
    displaySettings.m_visualizeWhitespace = on;
    m_ui->previewTextEdit->setDisplaySettings(displaySettings);
}

void QmlJSCodeStylePreferencesWidget::slotSettingsChanged()
{
    updatePreview();
}

void QmlJSCodeStylePreferencesWidget::updatePreview()
{
    QTextDocument *doc = m_ui->previewTextEdit->document();

    const TabSettings &ts = m_preferences
            ? m_preferences->currentTabSettings()
            : TextEditorSettings::codeStyle()->tabSettings();
    m_ui->previewTextEdit->textDocument()->setTabSettings(ts);
    CreatorCodeFormatter formatter(ts);
    formatter.invalidateCache(doc);

    QTextBlock block = doc->firstBlock();
    QTextCursor tc = m_ui->previewTextEdit->textCursor();
    tc.beginEditBlock();
    while (block.isValid()) {
        m_ui->previewTextEdit->textDocument()->indenter()->indentBlock(block, QChar::Null, ts);
        block = block.next();
    }
    tc.endEditBlock();
}

// ------------------ CppCodeStyleSettingsPage

QmlJSCodeStyleSettingsPage::QmlJSCodeStyleSettingsPage()
{
    setId(Constants::QML_JS_CODE_STYLE_SETTINGS_ID);
    setDisplayName(QCoreApplication::translate("QmlJSTools", Constants::QML_JS_CODE_STYLE_SETTINGS_NAME));
    setCategory(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
    setDisplayCategory(QCoreApplication::translate("QmlJSEditor", "Qt Quick"));
    setCategoryIconPath(":/qmljstools/images/settingscategory_qml.png");
}

QWidget *QmlJSCodeStyleSettingsPage::widget()
{
    if (!m_widget) {
        QmlJSCodeStylePreferences *originalPreferences
                = QmlJSToolsSettings::globalCodeStyle();
        m_preferences = new QmlJSCodeStylePreferences(m_widget);
        m_preferences->setDelegatingPool(originalPreferences->delegatingPool());
        m_preferences->setCodeStyleSettings(originalPreferences->codeStyleSettings());
        m_preferences->setTabSettings(originalPreferences->tabSettings());
        m_preferences->setCurrentDelegate(originalPreferences->currentDelegate());
        m_preferences->setId(originalPreferences->id());
        m_widget = new CodeStyleEditor(TextEditorSettings::codeStyleFactory(QmlJSTools::Constants::QML_JS_SETTINGS_ID),
                                       m_preferences);
    }
    return m_widget;
}

void QmlJSCodeStyleSettingsPage::apply()
{
    if (m_widget) {
        QSettings *s = Core::ICore::settings();

        QmlJSCodeStylePreferences *originalPreferences = QmlJSToolsSettings::globalCodeStyle();
        if (originalPreferences->codeStyleSettings() != m_preferences->codeStyleSettings()) {
            originalPreferences->setCodeStyleSettings(m_preferences->codeStyleSettings());
            originalPreferences->toSettings(QLatin1String(QmlJSTools::Constants::QML_JS_SETTINGS_ID), s);
        }
        if (originalPreferences->tabSettings() != m_preferences->tabSettings()) {
            originalPreferences->setTabSettings(m_preferences->tabSettings());
            originalPreferences->toSettings(QLatin1String(QmlJSTools::Constants::QML_JS_SETTINGS_ID), s);
        }
        if (originalPreferences->currentDelegate() != m_preferences->currentDelegate()) {
            originalPreferences->setCurrentDelegate(m_preferences->currentDelegate());
            originalPreferences->toSettings(QLatin1String(QmlJSTools::Constants::QML_JS_SETTINGS_ID), s);
        }
    }
}

void QmlJSCodeStyleSettingsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace QmlJSTools
