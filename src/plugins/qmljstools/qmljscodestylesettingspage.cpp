/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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
#include <texteditor/fontsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/displaysettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/codestyleeditor.h>
#include <extensionsystem/pluginmanager.h>
#include <qmldesigner/qmldesignerconstants.h>
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

    const QList<ISnippetProvider *> &providers =
        ExtensionSystem::PluginManager::getObjects<ISnippetProvider>();
    foreach (ISnippetProvider *provider, providers) {
        if (provider->groupId() == QLatin1String(QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID)) {
            provider->decorateEditor(m_ui->previewTextEdit);
            break;
        }
    }

    TextEditor::TextEditorSettings *textEditorSettings = TextEditorSettings::instance();
    decorateEditor(textEditorSettings->fontSettings());
    connect(textEditorSettings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
       this, SLOT(decorateEditor(TextEditor::FontSettings)));

    setVisualizeWhitespace(true);

    updatePreview();
}

QmlJSCodeStylePreferencesWidget::~QmlJSCodeStylePreferencesWidget()
{
    delete m_ui;
}

void QmlJSCodeStylePreferencesWidget::setPreferences(TextEditor::ICodeStylePreferences *preferences)
{
    m_preferences = preferences;
    m_ui->tabPreferencesWidget->setPreferences(preferences);
    if (m_preferences)
        connect(m_preferences, SIGNAL(currentTabSettingsChanged(TextEditor::TabSettings)),
                this, SLOT(slotSettingsChanged()));
    updatePreview();
}


QString QmlJSCodeStylePreferencesWidget::searchKeywords() const
{
    QString rc;
    QLatin1Char sep(' ');
    QTextStream(&rc)
       << sep << m_ui->tabPreferencesWidget->searchKeywords()
          ;
    rc.remove(QLatin1Char('&'));
    return rc;
}

void QmlJSCodeStylePreferencesWidget::decorateEditor(const TextEditor::FontSettings &fontSettings)
{
    const ISnippetProvider *provider = 0;
    const QList<ISnippetProvider *> &providers =
        ExtensionSystem::PluginManager::getObjects<ISnippetProvider>();
    foreach (const ISnippetProvider *current, providers) {
        if (current->groupId() == QLatin1String(QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID)) {
            provider = current;
            break;
        }
    }

    m_ui->previewTextEdit->setFontSettings(fontSettings);
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

    const TextEditor::TabSettings &ts = m_preferences
            ? m_preferences->currentTabSettings()
            : TextEditorSettings::instance()->codeStyle()->tabSettings();
    m_ui->previewTextEdit->setTabSettings(ts);
    CreatorCodeFormatter formatter(ts);
    formatter.invalidateCache(doc);

    QTextBlock block = doc->firstBlock();
    QTextCursor tc = m_ui->previewTextEdit->textCursor();
    tc.beginEditBlock();
    while (block.isValid()) {
        m_ui->previewTextEdit->indenter()->indentBlock(doc, block, QChar::Null, ts);

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
    setCategoryIcon(QLatin1String(QmlDesigner::Constants::SETTINGS_CATEGORY_QML_ICON));
}

QWidget *QmlJSCodeStyleSettingsPage::createPage(QWidget *parent)
{
    TextEditor::SimpleCodeStylePreferences *originalTabPreferences
            = QmlJSToolsSettings::globalCodeStyle();
    m_pageTabPreferences = new TextEditor::SimpleCodeStylePreferences(m_widget);
    m_pageTabPreferences->setDelegatingPool(originalTabPreferences->delegatingPool());
    m_pageTabPreferences->setTabSettings(originalTabPreferences->tabSettings());
    m_pageTabPreferences->setCurrentDelegate(originalTabPreferences->currentDelegate());
    m_pageTabPreferences->setId(originalTabPreferences->id());
    TextEditorSettings *settings = TextEditorSettings::instance();
    m_widget = new CodeStyleEditor(settings->codeStyleFactory(QmlJSTools::Constants::QML_JS_SETTINGS_ID),
                                   m_pageTabPreferences, parent);

    return m_widget;
}

void QmlJSCodeStyleSettingsPage::apply()
{
    if (m_widget) {
        QSettings *s = Core::ICore::settings();

        TextEditor::SimpleCodeStylePreferences *originalTabPreferences = QmlJSToolsSettings::globalCodeStyle();
        if (originalTabPreferences->tabSettings() != m_pageTabPreferences->tabSettings()) {
            originalTabPreferences->setTabSettings(m_pageTabPreferences->tabSettings());
            if (s)
                originalTabPreferences->toSettings(QLatin1String(QmlJSTools::Constants::QML_JS_SETTINGS_ID), s);
        }
        if (originalTabPreferences->currentDelegate() != m_pageTabPreferences->currentDelegate()) {
            originalTabPreferences->setCurrentDelegate(m_pageTabPreferences->currentDelegate());
            if (s)
                originalTabPreferences->toSettings(QLatin1String(QmlJSTools::Constants::QML_JS_SETTINGS_ID), s);
        }
    }
}

bool QmlJSCodeStyleSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace QmlJSTools
