/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "texteditorsettings.h"
#include "texteditorconstants.h"

#include "basetexteditor.h"
#include "behaviorsettings.h"
#include "behaviorsettingspage.h"
#include "completionsettings.h"
#include "displaysettings.h"
#include "displaysettingspage.h"
#include "fontsettingspage.h"
#include "typingsettings.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "extraencodingsettings.h"
#include "texteditorplugin.h"
#include "highlightersettingspage.h"
#include "snippetssettingspage.h"
#include "icodestylepreferences.h"
#include "icodestylepreferencesfactory.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QApplication>

using namespace TextEditor;
using namespace TextEditor::Constants;
using namespace TextEditor::Internal;

namespace TextEditor {
namespace Internal {

class TextEditorSettingsPrivate
{
public:
    FontSettingsPage *m_fontSettingsPage;
    BehaviorSettingsPage *m_behaviorSettingsPage;
    DisplaySettingsPage *m_displaySettingsPage;
    HighlighterSettingsPage *m_highlighterSettingsPage;
    SnippetsSettingsPage *m_snippetsSettingsPage;

    QMap<QString, ICodeStylePreferencesFactory *> m_languageToFactory;

    QMap<QString, ICodeStylePreferences *> m_languageToCodeStyle;
    QMap<QString, CodeStylePool *> m_languageToCodeStylePool;
    QMap<QString, QString> m_mimeTypeToLanguage;

    CompletionSettings m_completionSettings;

    void fontZoomRequested(int pointSize);
    void zoomResetRequested();
};

void TextEditorSettingsPrivate::fontZoomRequested(int zoom)
{
    FontSettings &fs = const_cast<FontSettings&>(m_fontSettingsPage->fontSettings());
    fs.setFontZoom(qMax(10, fs.fontZoom() + zoom));
    m_fontSettingsPage->saveSettings();
}

void TextEditorSettingsPrivate::zoomResetRequested()
{
    FontSettings &fs = const_cast<FontSettings&>(m_fontSettingsPage->fontSettings());
    fs.setFontZoom(100);
    m_fontSettingsPage->saveSettings();
}

} // namespace Internal
} // namespace TextEditor


TextEditorSettings *TextEditorSettings::m_instance = 0;

TextEditorSettings::TextEditorSettings(QObject *parent)
    : QObject(parent)
    , m_d(new Internal::TextEditorSettingsPrivate)
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;

    // Note: default background colors are coming from FormatDescription::background()

    // Add font preference page
    FormatDescriptions formatDescriptions;
    formatDescriptions.append(FormatDescription(C_TEXT, tr("Text")));

    // Special categories
    const QPalette p = QApplication::palette();
    formatDescriptions.append(FormatDescription(C_LINK, tr("Link"), Qt::blue));
    formatDescriptions.append(FormatDescription(C_SELECTION, tr("Selection"), p.color(QPalette::HighlightedText)));
    formatDescriptions.append(FormatDescription(C_LINE_NUMBER, tr("Line Number")));
    formatDescriptions.append(FormatDescription(C_SEARCH_RESULT, tr("Search Result")));
    formatDescriptions.append(FormatDescription(C_SEARCH_SCOPE, tr("Search Scope")));
    formatDescriptions.append(FormatDescription(C_PARENTHESES, tr("Parentheses")));
    formatDescriptions.append(FormatDescription(C_CURRENT_LINE, tr("Current Line")));

    FormatDescription currentLineNumber = FormatDescription(C_CURRENT_LINE_NUMBER, tr("Current Line Number"), Qt::darkGray);
    currentLineNumber.format().setBold(true);
    formatDescriptions.append(currentLineNumber);

    formatDescriptions.append(FormatDescription(C_OCCURRENCES, tr("Occurrences")));
    formatDescriptions.append(FormatDescription(C_OCCURRENCES_UNUSED, tr("Unused Occurrence")));
    formatDescriptions.append(FormatDescription(C_OCCURRENCES_RENAME, tr("Renaming Occurrence")));

    // Standard categories
    formatDescriptions.append(FormatDescription(C_NUMBER, tr("Number"), Qt::darkBlue));
    formatDescriptions.append(FormatDescription(C_STRING, tr("String"), Qt::darkGreen));
    formatDescriptions.append(FormatDescription(C_TYPE, tr("Type"), Qt::darkMagenta));
    formatDescriptions.append(FormatDescription(C_LOCAL, tr("Local")));
    formatDescriptions.append(FormatDescription(C_FIELD, tr("Field"), Qt::darkRed));
    formatDescriptions.append(FormatDescription(C_STATIC, tr("Static"), Qt::darkMagenta));

    Format functionFormat;
    functionFormat.setForeground(QColor(60, 60, 60)); // very dark grey
    formatDescriptions.append(FormatDescription(C_FUNCTION, tr("Function"), functionFormat));
    functionFormat.setItalic(true);
    formatDescriptions.append(FormatDescription(C_VIRTUAL_METHOD, tr("Virtual Method"), functionFormat));

    formatDescriptions.append(FormatDescription(C_BINDING, tr("QML Binding"), Qt::darkRed));

    Format qmlLocalNameFormat;
    qmlLocalNameFormat.setItalic(true);
    formatDescriptions.append(FormatDescription(C_QML_LOCAL_ID, tr("QML Local Id"), qmlLocalNameFormat));
    formatDescriptions.append(FormatDescription(C_QML_ROOT_OBJECT_PROPERTY, tr("QML Root Object Property"), qmlLocalNameFormat));
    formatDescriptions.append(FormatDescription(C_QML_SCOPE_OBJECT_PROPERTY, tr("QML Scope Object Property"), qmlLocalNameFormat));
    formatDescriptions.append(FormatDescription(C_QML_STATE_NAME, tr("QML State Name"), qmlLocalNameFormat));

    formatDescriptions.append(FormatDescription(C_QML_TYPE_ID, tr("QML Type Name"), Qt::darkMagenta));

    Format qmlExternalNameFormat = qmlLocalNameFormat;
    qmlExternalNameFormat.setForeground(Qt::darkBlue);
    formatDescriptions.append(FormatDescription(C_QML_EXTERNAL_ID, tr("QML External Id"), qmlExternalNameFormat));
    formatDescriptions.append(FormatDescription(C_QML_EXTERNAL_OBJECT_PROPERTY, tr("QML External Object Property"), qmlExternalNameFormat));

    Format jsLocalFormat;
    jsLocalFormat.setForeground(QColor(41, 133, 199)); // very light blue
    jsLocalFormat.setItalic(true);
    formatDescriptions.append(FormatDescription(C_JS_SCOPE_VAR, tr("JavaScript Scope Var"), jsLocalFormat));

    Format jsGlobalFormat;
    jsGlobalFormat.setForeground(QColor(0, 85, 175)); // light blue
    jsGlobalFormat.setItalic(true);
    formatDescriptions.append(FormatDescription(C_JS_IMPORT_VAR, tr("JavaScript Import"), jsGlobalFormat));
    formatDescriptions.append(FormatDescription(C_JS_GLOBAL_VAR, tr("JavaScript Global Variable"), jsGlobalFormat));

    formatDescriptions.append(FormatDescription(C_KEYWORD, tr("Keyword"), Qt::darkYellow));
    formatDescriptions.append(FormatDescription(C_OPERATOR, tr("Operator")));
    formatDescriptions.append(FormatDescription(C_PREPROCESSOR, tr("Preprocessor"), Qt::darkBlue));
    formatDescriptions.append(FormatDescription(C_LABEL, tr("Label"), Qt::darkRed));
    formatDescriptions.append(FormatDescription(C_COMMENT, tr("Comment"), Qt::darkGreen));
    formatDescriptions.append(FormatDescription(C_DOXYGEN_COMMENT, tr("Doxygen Comment"), Qt::darkBlue));
    formatDescriptions.append(FormatDescription(C_DOXYGEN_TAG, tr("Doxygen Tag"), Qt::blue));
    formatDescriptions.append(FormatDescription(C_VISUAL_WHITESPACE, tr("Visual Whitespace"), Qt::lightGray));
    formatDescriptions.append(FormatDescription(C_DISABLED_CODE, tr("Disabled Code")));

    // Diff categories
    formatDescriptions.append(FormatDescription(C_ADDED_LINE, tr("Added Line"), QColor(0, 170, 0)));
    formatDescriptions.append(FormatDescription(C_REMOVED_LINE, tr("Removed Line"), Qt::red));
    formatDescriptions.append(FormatDescription(C_DIFF_FILE, tr("Diff File"), Qt::darkBlue));
    formatDescriptions.append(FormatDescription(C_DIFF_LOCATION, tr("Diff Location"), Qt::blue));

    m_d->m_fontSettingsPage = new FontSettingsPage(formatDescriptions,
                                                   QLatin1String(Constants::TEXT_EDITOR_FONT_SETTINGS),
                                                   this);
    ExtensionSystem::PluginManager::addObject(m_d->m_fontSettingsPage);

    // Add the GUI used to configure the tab, storage and interaction settings
    TextEditor::BehaviorSettingsPageParameters behaviorSettingsPageParameters;
    behaviorSettingsPageParameters.id = QLatin1String(Constants::TEXT_EDITOR_BEHAVIOR_SETTINGS);
    behaviorSettingsPageParameters.displayName = tr("Behavior");
    behaviorSettingsPageParameters.settingsPrefix = QLatin1String("text");
    m_d->m_behaviorSettingsPage = new BehaviorSettingsPage(behaviorSettingsPageParameters, this);
    ExtensionSystem::PluginManager::addObject(m_d->m_behaviorSettingsPage);

    TextEditor::DisplaySettingsPageParameters displaySettingsPageParameters;
    displaySettingsPageParameters.id = QLatin1String(Constants::TEXT_EDITOR_DISPLAY_SETTINGS),
    displaySettingsPageParameters.displayName = tr("Display");
    displaySettingsPageParameters.settingsPrefix = QLatin1String("text");
    m_d->m_displaySettingsPage = new DisplaySettingsPage(displaySettingsPageParameters, this);
    ExtensionSystem::PluginManager::addObject(m_d->m_displaySettingsPage);

    m_d->m_highlighterSettingsPage =
        new HighlighterSettingsPage(QLatin1String(Constants::TEXT_EDITOR_HIGHLIGHTER_SETTINGS), this);
    ExtensionSystem::PluginManager::addObject(m_d->m_highlighterSettingsPage);

    m_d->m_snippetsSettingsPage =
        new SnippetsSettingsPage(QLatin1String(Constants::TEXT_EDITOR_SNIPPETS_SETTINGS), this);
    ExtensionSystem::PluginManager::addObject(m_d->m_snippetsSettingsPage);

    connect(m_d->m_fontSettingsPage, SIGNAL(changed(TextEditor::FontSettings)),
            this, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)));
    connect(m_d->m_behaviorSettingsPage, SIGNAL(typingSettingsChanged(TextEditor::TypingSettings)),
            this, SIGNAL(typingSettingsChanged(TextEditor::TypingSettings)));
    connect(m_d->m_behaviorSettingsPage, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)),
            this, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)));
    connect(m_d->m_behaviorSettingsPage, SIGNAL(behaviorSettingsChanged(TextEditor::BehaviorSettings)),
            this, SIGNAL(behaviorSettingsChanged(TextEditor::BehaviorSettings)));
    connect(m_d->m_displaySettingsPage, SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)),
            this, SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)));

    // TODO: Move these settings to TextEditor category
    if (QSettings *s = Core::ICore::settings())
        m_d->m_completionSettings.fromSettings(QLatin1String("CppTools/"), s);
}

TextEditorSettings::~TextEditorSettings()
{
    ExtensionSystem::PluginManager::removeObject(m_d->m_fontSettingsPage);
    ExtensionSystem::PluginManager::removeObject(m_d->m_behaviorSettingsPage);
    ExtensionSystem::PluginManager::removeObject(m_d->m_displaySettingsPage);
    ExtensionSystem::PluginManager::removeObject(m_d->m_highlighterSettingsPage);
    ExtensionSystem::PluginManager::removeObject(m_d->m_snippetsSettingsPage);

    delete m_d;

    m_instance = 0;
}

TextEditorSettings *TextEditorSettings::instance()
{
    return m_instance;
}

/**
 * Initializes editor settings. Also connects signals to keep them up to date
 * when they are changed.
 */
void TextEditorSettings::initializeEditor(BaseTextEditorWidget *editor)
{
    // Connect to settings change signals
    connect(this, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            editor, SLOT(setFontSettingsIfVisible(TextEditor::FontSettings)));
    connect(this, SIGNAL(typingSettingsChanged(TextEditor::TypingSettings)),
            editor, SLOT(setTypingSettings(TextEditor::TypingSettings)));
    connect(this, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)),
            editor, SLOT(setStorageSettings(TextEditor::StorageSettings)));
    connect(this, SIGNAL(behaviorSettingsChanged(TextEditor::BehaviorSettings)),
            editor, SLOT(setBehaviorSettings(TextEditor::BehaviorSettings)));
    connect(this, SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)),
            editor, SLOT(setDisplaySettings(TextEditor::DisplaySettings)));
    connect(this, SIGNAL(completionSettingsChanged(TextEditor::CompletionSettings)),
            editor, SLOT(setCompletionSettings(TextEditor::CompletionSettings)));
    connect(this, SIGNAL(extraEncodingSettingsChanged(TextEditor::ExtraEncodingSettings)),
            editor, SLOT(setExtraEncodingSettings(TextEditor::ExtraEncodingSettings)));

    connect(editor, SIGNAL(requestFontZoom(int)),
            this, SLOT(fontZoomRequested(int)));
    connect(editor, SIGNAL(requestZoomReset()),
            this, SLOT(zoomResetRequested()));

    // Apply current settings (tab settings depend on font settings)
    editor->setFontSettings(fontSettings());
    editor->setTabSettings(codeStyle()->tabSettings());
    editor->setTypingSettings(typingSettings());
    editor->setStorageSettings(storageSettings());
    editor->setBehaviorSettings(behaviorSettings());
    editor->setDisplaySettings(displaySettings());
    editor->setCompletionSettings(completionSettings());
    editor->setExtraEncodingSettings(extraEncodingSettings());
    editor->setCodeStyle(codeStyle(editor->languageSettingsId()));
}

const FontSettings &TextEditorSettings::fontSettings() const
{
    return m_d->m_fontSettingsPage->fontSettings();
}

const TypingSettings &TextEditorSettings::typingSettings() const
{
    return m_d->m_behaviorSettingsPage->typingSettings();
}

const StorageSettings &TextEditorSettings::storageSettings() const
{
    return m_d->m_behaviorSettingsPage->storageSettings();
}

const BehaviorSettings &TextEditorSettings::behaviorSettings() const
{
    return m_d->m_behaviorSettingsPage->behaviorSettings();
}

const DisplaySettings &TextEditorSettings::displaySettings() const
{
    return m_d->m_displaySettingsPage->displaySettings();
}

const CompletionSettings &TextEditorSettings::completionSettings() const
{
    return m_d->m_completionSettings;
}

const HighlighterSettings &TextEditorSettings::highlighterSettings() const
{
    return m_d->m_highlighterSettingsPage->highlighterSettings();
}

const ExtraEncodingSettings &TextEditorSettings::extraEncodingSettings() const
{
    return m_d->m_behaviorSettingsPage->extraEncodingSettings();
}

void TextEditorSettings::setCompletionSettings(const TextEditor::CompletionSettings &settings)
{
    if (m_d->m_completionSettings == settings)
        return;

    m_d->m_completionSettings = settings;
    if (QSettings *s = Core::ICore::settings())
        m_d->m_completionSettings.toSettings(QLatin1String("CppTools/"), s);

    emit completionSettingsChanged(m_d->m_completionSettings);
}

void TextEditorSettings::registerCodeStyleFactory(ICodeStylePreferencesFactory *factory)
{
    m_d->m_languageToFactory.insert(factory->languageId(), factory);
}

QMap<QString, ICodeStylePreferencesFactory *> TextEditorSettings::codeStyleFactories() const
{
    return m_d->m_languageToFactory;
}

ICodeStylePreferencesFactory *TextEditorSettings::codeStyleFactory(const QString &languageId) const
{
    return m_d->m_languageToFactory.value(languageId);
}

ICodeStylePreferences *TextEditorSettings::codeStyle() const
{
    return m_d->m_behaviorSettingsPage->codeStyle();
}

ICodeStylePreferences *TextEditorSettings::codeStyle(const QString &languageId) const
{
    return m_d->m_languageToCodeStyle.value(languageId, codeStyle());
}

QMap<QString, ICodeStylePreferences *> TextEditorSettings::codeStyles() const
{
    return m_d->m_languageToCodeStyle;
}

void TextEditorSettings::registerCodeStyle(const QString &languageId, ICodeStylePreferences *prefs)
{
    m_d->m_languageToCodeStyle.insert(languageId, prefs);
}

CodeStylePool *TextEditorSettings::codeStylePool() const
{
    return m_d->m_behaviorSettingsPage->codeStylePool();
}

CodeStylePool *TextEditorSettings::codeStylePool(const QString &languageId) const
{
    return m_d->m_languageToCodeStylePool.value(languageId);
}

void TextEditorSettings::registerCodeStylePool(const QString &languageId, CodeStylePool *pool)
{
    m_d->m_languageToCodeStylePool.insert(languageId, pool);
}

void TextEditorSettings::registerMimeTypeForLanguageId(const QString &mimeType, const QString &languageId)
{
    m_d->m_mimeTypeToLanguage.insert(mimeType, languageId);
}

QString TextEditorSettings::languageId(const QString &mimeType) const
{
    return m_d->m_mimeTypeToLanguage.value(mimeType);
}

#include "moc_texteditorsettings.cpp"
