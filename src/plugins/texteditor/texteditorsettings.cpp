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

#include "texteditorsettings.h"

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

    QMap<Core::Id, ICodeStylePreferencesFactory *> m_languageToFactory;

    QMap<Core::Id, ICodeStylePreferences *> m_languageToCodeStyle;
    QMap<Core::Id, CodeStylePool *> m_languageToCodeStylePool;
    QMap<QString, Core::Id> m_mimeTypeToLanguage;

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
    FormatDescriptions formatDescr;
    formatDescr.append(FormatDescription(C_TEXT, tr("Text"), tr("Generic text.\nApplied to "
                                                                "text, if no other "
                                                                "rules matching.")));

    // Special categories
    const QPalette p = QApplication::palette();
    formatDescr.append(FormatDescription(C_LINK, tr("Link"),
                                         tr("Links that follow symbol under cursor."), Qt::blue));
    formatDescr.append(FormatDescription(C_SELECTION, tr("Selection"), tr("Selected text."),
                                         p.color(QPalette::HighlightedText)));
    formatDescr.append(FormatDescription(C_LINE_NUMBER, tr("Line Number"),
                                         tr("Line numbers located on the "
                                            "left side of the editor.")));
    formatDescr.append(FormatDescription(C_SEARCH_RESULT, tr("Search Result"),
                                         tr("Highlighted search results inside the editor.")));
    formatDescr.append(FormatDescription(C_SEARCH_SCOPE, tr("Search Scope"),
                                         tr("Section where the pattern is searched in.")));
    formatDescr.append(FormatDescription(C_PARENTHESES, tr("Parentheses"),
                                         tr("Displayed when matching parentheses, square brackets "
                                            "or curly brackets are found.")));
    formatDescr.append(FormatDescription(C_CURRENT_LINE, tr("Current Line"),
                                         tr("Line where the cursor is placed in.")));

    FormatDescription currentLineNumber =
            FormatDescription(C_CURRENT_LINE_NUMBER, tr("Current Line Number"),
                              tr("Line number located on the left side of the "
                                 "editor where the cursor is placed in."), Qt::darkGray);
    currentLineNumber.format().setBold(true);
    formatDescr.append(currentLineNumber);


    formatDescr.append(FormatDescription(C_OCCURRENCES, tr("Occurrences"),
                                         tr("Occurrences of the symbol under the cursor.\n"
                                            "(Only the background will be applied.)")));
    formatDescr.append(FormatDescription(C_OCCURRENCES_UNUSED, tr("Unused Occurrence"),
                                         tr("Occurrences of unused variables.")));
    formatDescr.append(FormatDescription(C_OCCURRENCES_RENAME, tr("Renaming Occurrence"),
                                         tr("Occurrences of a symbol that will be renamed.")));

    // Standard categories
    formatDescr.append(FormatDescription(C_NUMBER, tr("Number"), tr("Number literal."),
                                         Qt::darkBlue));
    formatDescr.append(FormatDescription(C_STRING, tr("String"),
                                         tr("Character and string literals."), Qt::darkGreen));
    formatDescr.append(FormatDescription(C_TYPE, tr("Type"), tr("Name of a type."),
                                         Qt::darkMagenta));
    formatDescr.append(FormatDescription(C_LOCAL, tr("Local"), tr("Local variables.")));
    formatDescr.append(FormatDescription(C_FIELD, tr("Field"),
                                         tr("Class' data members."), Qt::darkRed));
    formatDescr.append(FormatDescription(C_ENUMERATION, tr("Enumeration"),
                                         tr("Applied to enumeration items."), Qt::darkMagenta));

    Format functionFormat;
    formatDescr.append(FormatDescription(C_FUNCTION, tr("Function"), tr("Name of a function."),
                                         functionFormat));
    functionFormat.setItalic(true);
    formatDescr.append(FormatDescription(C_VIRTUAL_METHOD, tr("Virtual Method"),
                                         tr("Name of method declared as virtual."),
                                         functionFormat));

    formatDescr.append(FormatDescription(C_BINDING, tr("QML Binding"),
                                         tr("QML item property, that allows a "
                                            "binding to another property."),
                                         Qt::darkRed));

    Format qmlLocalNameFormat;
    qmlLocalNameFormat.setItalic(true);
    formatDescr.append(FormatDescription(C_QML_LOCAL_ID, tr("QML Local Id"),
                                         tr("QML item id within a QML file."), qmlLocalNameFormat));
    formatDescr.append(FormatDescription(C_QML_ROOT_OBJECT_PROPERTY,
                                         tr("QML Root Object Property"),
                                         tr("QML property of a parent item."), qmlLocalNameFormat));
    formatDescr.append(FormatDescription(C_QML_SCOPE_OBJECT_PROPERTY,
                                         tr("QML Scope Object Property"),
                                         tr("Property of the same QML item."), qmlLocalNameFormat));
    formatDescr.append(FormatDescription(C_QML_STATE_NAME, tr("QML State Name"),
                                         tr("Name of a QML state."), qmlLocalNameFormat));

    formatDescr.append(FormatDescription(C_QML_TYPE_ID, tr("QML Type Name"),
                                         tr("Name of a QML type."), Qt::darkMagenta));

    Format qmlExternalNameFormat = qmlLocalNameFormat;
    qmlExternalNameFormat.setForeground(Qt::darkBlue);
    formatDescr.append(FormatDescription(C_QML_EXTERNAL_ID, tr("QML External Id"),
                                         tr("QML id defined in another QML file."),
                                         qmlExternalNameFormat));
    formatDescr.append(FormatDescription(C_QML_EXTERNAL_OBJECT_PROPERTY,
                                         tr("QML External Object Property"),
                                         tr("QML property defined in another QML file."),
                                         qmlExternalNameFormat));

    Format jsLocalFormat;
    jsLocalFormat.setForeground(QColor(41, 133, 199)); // very light blue
    jsLocalFormat.setItalic(true);
    formatDescr.append(FormatDescription(C_JS_SCOPE_VAR, tr("JavaScript Scope Var"),
                                         tr("Variables defined inside the JavaScript file."),
                                         jsLocalFormat));

    Format jsGlobalFormat;
    jsGlobalFormat.setForeground(QColor(0, 85, 175)); // light blue
    jsGlobalFormat.setItalic(true);
    formatDescr.append(FormatDescription(C_JS_IMPORT_VAR, tr("JavaScript Import"),
                                         tr("Name of a JavaScript import inside a QML file."),
                                         jsGlobalFormat));
    formatDescr.append(FormatDescription(C_JS_GLOBAL_VAR, tr("JavaScript Global Variable"),
                                         tr("Variables defined outside the script."),
                                         jsGlobalFormat));

    formatDescr.append(FormatDescription(C_KEYWORD, tr("Keyword"),
                                         tr("Reserved keywords of the programming language."),
                                         Qt::darkYellow));
    formatDescr.append(FormatDescription(C_OPERATOR, tr("Operator"),
                                         tr("Operators. (For example operator++ operator-=)")));
    formatDescr.append(FormatDescription(C_PREPROCESSOR, tr("Preprocessor"),
                                         tr("Preprocessor directives."), Qt::darkBlue));
    formatDescr.append(FormatDescription(C_LABEL, tr("Label"), tr("Labels for goto statements."),
                                         Qt::darkRed));
    formatDescr.append(FormatDescription(C_COMMENT, tr("Comment"),
                                         tr("All style of comments except Doxygen comments."),
                                         Qt::darkGreen));
    formatDescr.append(FormatDescription(C_DOXYGEN_COMMENT, tr("Doxygen Comment"),
                                         tr("Doxygen comments."), Qt::darkBlue));
    formatDescr.append(FormatDescription(C_DOXYGEN_TAG, tr("Doxygen Tag"), tr("Doxygen tags."),
                                         Qt::blue));
    formatDescr.append(FormatDescription(C_VISUAL_WHITESPACE, tr("Visual Whitespace"),
                                         tr("Whitespace\nWill not be applied to whitespace "
                                            "in comments and strings."), Qt::lightGray));
    formatDescr.append(FormatDescription(C_DISABLED_CODE, tr("Disabled Code"),
                                         tr("Code disabled by preprocessor directives.")));

    // Diff categories
    formatDescr.append(FormatDescription(C_ADDED_LINE, tr("Added Line"),
                                         tr("Applied to added lines in differences "
                                            "(in diff editor)."), QColor(0, 170, 0)));
    formatDescr.append(FormatDescription(C_REMOVED_LINE, tr("Removed Line"),
                                         tr("Applied to removed lines "
                                            "in differences (in diff editor)."), Qt::red));
    formatDescr.append(FormatDescription(C_DIFF_FILE, tr("Diff File"),
                                         tr("Compared files (in diff editor)."), Qt::darkBlue));
    formatDescr.append(FormatDescription(C_DIFF_LOCATION, tr("Diff Location"),
                                         tr("Location in the files where the difference is "
                                            "(in diff editor)."), Qt::blue));

    m_d->m_fontSettingsPage = new FontSettingsPage(formatDescr,
                                                   Constants::TEXT_EDITOR_FONT_SETTINGS,
                                                   this);
    ExtensionSystem::PluginManager::addObject(m_d->m_fontSettingsPage);

    // Add the GUI used to configure the tab, storage and interaction settings
    TextEditor::BehaviorSettingsPageParameters behaviorSettingsPageParameters;
    behaviorSettingsPageParameters.id = Constants::TEXT_EDITOR_BEHAVIOR_SETTINGS;
    behaviorSettingsPageParameters.displayName = tr("Behavior");
    behaviorSettingsPageParameters.settingsPrefix = QLatin1String("text");
    m_d->m_behaviorSettingsPage = new BehaviorSettingsPage(behaviorSettingsPageParameters, this);
    ExtensionSystem::PluginManager::addObject(m_d->m_behaviorSettingsPage);

    TextEditor::DisplaySettingsPageParameters displaySettingsPageParameters;
    displaySettingsPageParameters.id = Constants::TEXT_EDITOR_DISPLAY_SETTINGS;
    displaySettingsPageParameters.displayName = tr("Display");
    displaySettingsPageParameters.settingsPrefix = QLatin1String("text");
    m_d->m_displaySettingsPage = new DisplaySettingsPage(displaySettingsPageParameters, this);
    ExtensionSystem::PluginManager::addObject(m_d->m_displaySettingsPage);

    m_d->m_highlighterSettingsPage =
        new HighlighterSettingsPage(Constants::TEXT_EDITOR_HIGHLIGHTER_SETTINGS, this);
    ExtensionSystem::PluginManager::addObject(m_d->m_highlighterSettingsPage);

    m_d->m_snippetsSettingsPage =
        new SnippetsSettingsPage(Constants::TEXT_EDITOR_SNIPPETS_SETTINGS, this);
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
    m_d->m_completionSettings.fromSettings(QLatin1String("CppTools/"), Core::ICore::settings());
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
    m_d->m_completionSettings.toSettings(QLatin1String("CppTools/"), Core::ICore::settings());

    emit completionSettingsChanged(m_d->m_completionSettings);
}

void TextEditorSettings::registerCodeStyleFactory(ICodeStylePreferencesFactory *factory)
{
    m_d->m_languageToFactory.insert(factory->languageId(), factory);
}

void TextEditorSettings::unregisterCodeStyleFactory(Core::Id languageId)
{
    m_d->m_languageToFactory.remove(languageId);
}

QMap<Core::Id, ICodeStylePreferencesFactory *> TextEditorSettings::codeStyleFactories() const
{
    return m_d->m_languageToFactory;
}

ICodeStylePreferencesFactory *TextEditorSettings::codeStyleFactory(Core::Id languageId) const
{
    return m_d->m_languageToFactory.value(languageId);
}

ICodeStylePreferences *TextEditorSettings::codeStyle() const
{
    return m_d->m_behaviorSettingsPage->codeStyle();
}

ICodeStylePreferences *TextEditorSettings::codeStyle(Core::Id languageId) const
{
    return m_d->m_languageToCodeStyle.value(languageId, codeStyle());
}

QMap<Core::Id, ICodeStylePreferences *> TextEditorSettings::codeStyles() const
{
    return m_d->m_languageToCodeStyle;
}

void TextEditorSettings::registerCodeStyle(Core::Id languageId, ICodeStylePreferences *prefs)
{
    m_d->m_languageToCodeStyle.insert(languageId, prefs);
}

void TextEditorSettings::unregisterCodeStyle(Core::Id languageId)
{
    m_d->m_languageToCodeStyle.remove(languageId);
}

CodeStylePool *TextEditorSettings::codeStylePool() const
{
    return m_d->m_behaviorSettingsPage->codeStylePool();
}

CodeStylePool *TextEditorSettings::codeStylePool(Core::Id languageId) const
{
    return m_d->m_languageToCodeStylePool.value(languageId);
}

void TextEditorSettings::registerCodeStylePool(Core::Id languageId, CodeStylePool *pool)
{
    m_d->m_languageToCodeStylePool.insert(languageId, pool);
}

void TextEditorSettings::unregisterCodeStylePool(Core::Id languageId)
{
    m_d->m_languageToCodeStylePool.remove(languageId);
}

void TextEditorSettings::registerMimeTypeForLanguageId(const QString &mimeType, Core::Id languageId)
{
    m_d->m_mimeTypeToLanguage.insert(mimeType, languageId);
}

Core::Id TextEditorSettings::languageId(const QString &mimeType) const
{
    return m_d->m_mimeTypeToLanguage.value(mimeType);
}

#include "moc_texteditorsettings.cpp"
