/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmljscodestylesettingspage.h"
#include "ui_qmljscodestylesettingspage.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolssettings.h"
#include "qmljsindenter.h"
#include "qmljsqtstylecodeformatter.h"

#include <texteditor/snippets/isnippetprovider.h>
#include <texteditor/fontsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/tabpreferences.h>
#include <texteditor/displaysettings.h>
#include <texteditor/texteditorsettings.h>
#include <extensionsystem/pluginmanager.h>
#include <qmldesigner/qmldesignerconstants.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <coreplugin/icore.h>

#include <QtCore/QTextStream>

using namespace TextEditor;

namespace QmlJSTools {
namespace Internal {

// ------------------ CppCodeStyleSettingsWidget

QmlJSCodeStylePreferencesWidget::QmlJSCodeStylePreferencesWidget(QWidget *parent) :
    QWidget(parent),
    m_tabPreferences(0),
    m_ui(new Ui::QmlJSCodeStyleSettingsPage)
{
    m_ui->setupUi(this);

    const QList<ISnippetProvider *> &providers =
        ExtensionSystem::PluginManager::instance()->getObjects<ISnippetProvider>();
    foreach (ISnippetProvider *provider, providers) {
        if (provider->groupId() == QLatin1String(QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID)) {
            provider->decorateEditor(m_ui->previewTextEdit);
            break;
        }
    }
    TextEditor::TextEditorSettings *settings = TextEditorSettings::instance();
    setFontSettings(settings->fontSettings());
    connect(settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
       this, SLOT(setFontSettings(TextEditor::FontSettings)));

    setVisualizeWhitespace(true);

    updatePreview();
}

QmlJSCodeStylePreferencesWidget::~QmlJSCodeStylePreferencesWidget()
{
    delete m_ui;
}

void QmlJSCodeStylePreferencesWidget::setTabPreferences(TextEditor::TabPreferences *tabPreferences)
{
    m_tabPreferences = tabPreferences;
    m_ui->tabPreferencesWidget->setTabPreferences(tabPreferences);
    connect(m_tabPreferences, SIGNAL(currentSettingsChanged(TextEditor::TabSettings)),
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

void QmlJSCodeStylePreferencesWidget::setFontSettings(const TextEditor::FontSettings &fontSettings)
{
    m_ui->previewTextEdit->setFont(fontSettings.font());
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

    const TextEditor::TabSettings &ts = m_tabPreferences
            ? m_tabPreferences->currentSettings()
            : TextEditorSettings::instance()->tabPreferences()->settings();
    m_ui->previewTextEdit->setTabSettings(ts);
    QtStyleCodeFormatter formatter(ts);
    formatter.invalidateCache(doc);

    QTextBlock block = doc->firstBlock();
    QTextCursor tc = m_ui->previewTextEdit->textCursor();
    tc.beginEditBlock();
    while (block.isValid()) {
        int depth = formatter.indentFor(block);
        ts.indentLine(block, depth);
        formatter.updateLineStateChange(block);

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
}

QmlJSCodeStyleSettingsPage::~QmlJSCodeStyleSettingsPage()
{
}

QString QmlJSCodeStyleSettingsPage::id() const
{
    return QLatin1String(Constants::QML_JS_CODE_STYLE_SETTINGS_ID);
}

QString QmlJSCodeStyleSettingsPage::displayName() const
{
    return QCoreApplication::translate("QmlJSTools", Constants::QML_JS_CODE_STYLE_SETTINGS_NAME);
}

QString QmlJSCodeStyleSettingsPage::category() const
{
    return QLatin1String(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
}

QString QmlJSCodeStyleSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("QmlJSEditor", QmlJSEditor::Constants::SETTINGS_TR_CATEGORY_QML);
}

QIcon QmlJSCodeStyleSettingsPage::categoryIcon() const
{
    return QIcon(QLatin1String(QmlDesigner::Constants::SETTINGS_CATEGORY_QML_ICON));
}

QWidget *QmlJSCodeStyleSettingsPage::createPage(QWidget *parent)
{
    m_widget = new QmlJSCodeStylePreferencesWidget(parent);

    TextEditor::TabPreferences *originalTabPreferences
            = QmlJSToolsSettings::instance()->tabPreferences();
    QList<TextEditor::IFallbackPreferences *> originalTabFallbacks = originalTabPreferences->fallbacks();
    m_pageTabPreferences = new TextEditor::TabPreferences(originalTabFallbacks, m_widget);
    for (int i = 0; i < originalTabFallbacks.count(); i++) {
        TextEditor::IFallbackPreferences *fallback = originalTabFallbacks.at(i);
        m_pageTabPreferences->setFallbackEnabled(fallback, originalTabPreferences->isFallbackEnabled(fallback));
    }
    m_pageTabPreferences->setSettings(originalTabPreferences->settings());
    m_pageTabPreferences->setCurrentFallback(originalTabPreferences->currentFallback());
    m_widget->setTabPreferences(m_pageTabPreferences);

    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void QmlJSCodeStyleSettingsPage::apply()
{
    if (m_widget) {
        QSettings *s = Core::ICore::instance()->settings();

        TextEditor::TabPreferences *originalTabPreferences = QmlJSToolsSettings::instance()->tabPreferences();
        if (originalTabPreferences->settings() != m_pageTabPreferences->settings()) {
            originalTabPreferences->setSettings(m_pageTabPreferences->settings());
            if (s)
                originalTabPreferences->toSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID, s);
        }
        if (originalTabPreferences->currentFallback() != m_pageTabPreferences->currentFallback()) {
            originalTabPreferences->setCurrentFallback(m_pageTabPreferences->currentFallback());
            if (s)
                originalTabPreferences->toSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID, s);
        }
    }
}

bool QmlJSCodeStyleSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace QmlJSTools
