/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmljscodestylesettingspage.h"
#include "ui_qmljscodestylesettingspage.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolssettings.h"
#include "qmljsindenter.h"
#include "qmljsqtstylecodeformatter.h"

#include <texteditor/snippets/isnippetprovider.h>
#include <texteditor/tabsettings.h>
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/displaysettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/codestyleeditor.h>
#include <extensionsystem/pluginmanager.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <coreplugin/icore.h>

#include <QTextStream>

using namespace TextEditor;

namespace QmlJSTools {
namespace Internal {

// ------------------ CppCodeStyleSettingsWidget

QmlJSCodeStylePreferencesWidget::QmlJSCodeStylePreferencesWidget(QWidget *parent) :
    QWidget(parent),
    m_preferences(0),
    m_ui(new Ui::QmlJSCodeStyleSettingsPage)
{
    m_ui->setupUi(this);

    ISnippetProvider *provider = ExtensionSystem::PluginManager::getObject<ISnippetProvider>(
        [](ISnippetProvider *provider) {
            return provider->groupId() == QLatin1String(QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID);
        });

    if (provider)
        provider->decorateEditor(m_ui->previewTextEdit);

    decorateEditor(TextEditorSettings::fontSettings());
    connect(TextEditorSettings::instance(), SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
       this, SLOT(decorateEditor(TextEditor::FontSettings)));

    setVisualizeWhitespace(true);

    updatePreview();
}

QmlJSCodeStylePreferencesWidget::~QmlJSCodeStylePreferencesWidget()
{
    delete m_ui;
}

void QmlJSCodeStylePreferencesWidget::setPreferences(ICodeStylePreferences *preferences)
{
    m_preferences = preferences;
    m_ui->tabPreferencesWidget->setPreferences(preferences);
    if (m_preferences)
        connect(m_preferences, &ICodeStylePreferences::currentTabSettingsChanged,
                this, &QmlJSCodeStylePreferencesWidget::slotSettingsChanged);
    updatePreview();
}


void QmlJSCodeStylePreferencesWidget::decorateEditor(const FontSettings &fontSettings)
{
    const ISnippetProvider *provider = ExtensionSystem::PluginManager::getObject<ISnippetProvider>(
        [](ISnippetProvider *current) {
            return current->groupId() == QLatin1String(QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID);
        });

    m_ui->previewTextEdit->textDocument()->setFontSettings(fontSettings);
    if (provider)
        provider->decorateEditor(m_ui->previewTextEdit);
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
        m_ui->previewTextEdit->textDocument()->indenter()
                ->indentBlock(doc, block, QChar::Null, ts);
        block = block.next();
    }
    tc.endEditBlock();
}

// ------------------ CppCodeStyleSettingsPage

QmlJSCodeStyleSettingsPage::QmlJSCodeStyleSettingsPage(/*QSharedPointer<CppFileSettings> &settings,*/
                     QWidget *parent) :
    Core::IOptionsPage(parent),
    m_pageTabPreferences(0)
{
    setId(Constants::QML_JS_CODE_STYLE_SETTINGS_ID);
    setDisplayName(QCoreApplication::translate("QmlJSTools", Constants::QML_JS_CODE_STYLE_SETTINGS_NAME));
    setCategory(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
    setDisplayCategory(QCoreApplication::translate("QmlJSEditor", QmlJSEditor::Constants::SETTINGS_TR_CATEGORY_QML));
    setCategoryIcon(QLatin1String(QmlJSTools::Constants::SETTINGS_CATEGORY_QML_ICON));
}

QWidget *QmlJSCodeStyleSettingsPage::widget()
{
    if (!m_widget) {
        SimpleCodeStylePreferences *originalTabPreferences
                = QmlJSToolsSettings::globalCodeStyle();
        m_pageTabPreferences = new SimpleCodeStylePreferences(m_widget);
        m_pageTabPreferences->setDelegatingPool(originalTabPreferences->delegatingPool());
        m_pageTabPreferences->setTabSettings(originalTabPreferences->tabSettings());
        m_pageTabPreferences->setCurrentDelegate(originalTabPreferences->currentDelegate());
        m_pageTabPreferences->setId(originalTabPreferences->id());
        m_widget = new CodeStyleEditor(TextEditorSettings::codeStyleFactory(QmlJSTools::Constants::QML_JS_SETTINGS_ID),
                                       m_pageTabPreferences);
    }
    return m_widget;
}

void QmlJSCodeStyleSettingsPage::apply()
{
    if (m_widget) {
        QSettings *s = Core::ICore::settings();

        SimpleCodeStylePreferences *originalTabPreferences = QmlJSToolsSettings::globalCodeStyle();
        if (originalTabPreferences->tabSettings() != m_pageTabPreferences->tabSettings()) {
            originalTabPreferences->setTabSettings(m_pageTabPreferences->tabSettings());
            originalTabPreferences->toSettings(QLatin1String(QmlJSTools::Constants::QML_JS_SETTINGS_ID), s);
        }
        if (originalTabPreferences->currentDelegate() != m_pageTabPreferences->currentDelegate()) {
            originalTabPreferences->setCurrentDelegate(m_pageTabPreferences->currentDelegate());
            originalTabPreferences->toSettings(QLatin1String(QmlJSTools::Constants::QML_JS_SETTINGS_ID), s);
        }
    }
}

void QmlJSCodeStyleSettingsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace QmlJSTools
