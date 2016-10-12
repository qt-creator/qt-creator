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

#include "texteditorsettings.h"

#include "fontsettings.h"
#include "texteditor.h"
#include "behaviorsettings.h"
#include "behaviorsettingspage.h"
#include "completionsettings.h"
#include "marginsettings.h"
#include "displaysettings.h"
#include "displaysettingspage.h"
#include "fontsettingspage.h"
#include "typingsettings.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "extraencodingsettings.h"
#include "icodestylepreferences.h"
#include "icodestylepreferencesfactory.h"
#include "completionsettingspage.h"
#include <texteditor/generichighlighter/highlightersettingspage.h>
#include <texteditor/snippets/snippetssettingspage.h>

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
    CompletionSettingsPage *m_completionSettingsPage;

    QMap<Core::Id, ICodeStylePreferencesFactory *> m_languageToFactory;

    QMap<Core::Id, ICodeStylePreferences *> m_languageToCodeStyle;
    QMap<Core::Id, CodeStylePool *> m_languageToCodeStylePool;
    QMap<QString, Core::Id> m_mimeTypeToLanguage;
};

} // namespace Internal
} // namespace TextEditor


static TextEditorSettingsPrivate *d = 0;
static TextEditorSettings *m_instance = 0;

TextEditorSettings::TextEditorSettings(QObject *parent)
    : QObject(parent)
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;
    d = new Internal::TextEditorSettingsPrivate;

    // Note: default background colors are coming from FormatDescription::background()

    // Add font preference page
    FormatDescriptions formatDescr;
    formatDescr.reserve(C_LAST_STYLE_SENTINEL);
    formatDescr.emplace_back(C_TEXT, tr("Text"), tr("Generic text.\nApplied to "
                                                    "text, if no other "
                                                    "rules matching."));

    // Special categories
    const QPalette p = QApplication::palette();
    formatDescr.emplace_back(C_LINK, tr("Link"),
                             tr("Links that follow symbol under cursor."), Qt::blue);
    formatDescr.emplace_back(C_SELECTION, tr("Selection"), tr("Selected text."),
                             p.color(QPalette::HighlightedText));
    formatDescr.emplace_back(C_LINE_NUMBER, tr("Line Number"),
                             tr("Line numbers located on the left side of the editor."),
                             FormatDescription::AllControlsExceptUnderline);
    formatDescr.emplace_back(C_SEARCH_RESULT, tr("Search Result"),
                             tr("Highlighted search results inside the editor."),
                             FormatDescription::ShowBackgroundControl);
    formatDescr.emplace_back(C_SEARCH_SCOPE, tr("Search Scope"),
                             tr("Section where the pattern is searched in."),
                             FormatDescription::ShowBackgroundControl);
    formatDescr.emplace_back(C_PARENTHESES, tr("Parentheses"),
                             tr("Displayed when matching parentheses, square brackets "
                                "or curly brackets are found."));
    formatDescr.emplace_back(C_PARENTHESES_MISMATCH, tr("Mismatched Parentheses"),
                             tr("Displayed when mismatched parentheses, "
                                "square brackets, or curly brackets are found."));
    formatDescr.emplace_back(C_AUTOCOMPLETE, tr("Auto Complete"),
                             tr("Displayed when a character is automatically inserted "
                                "like brackets or quotes."));
    formatDescr.emplace_back(C_CURRENT_LINE, tr("Current Line"),
                             tr("Line where the cursor is placed in."),
                             FormatDescription::ShowBackgroundControl);

    FormatDescription currentLineNumber(C_CURRENT_LINE_NUMBER,
                                        tr("Current Line Number"),
                                        tr("Line number located on the left side of the "
                                           "editor where the cursor is placed in."),
                                        Qt::darkGray,
                                        FormatDescription::AllControlsExceptUnderline);
    currentLineNumber.format().setBold(true);
    formatDescr.push_back(std::move(currentLineNumber));


    formatDescr.emplace_back(C_OCCURRENCES, tr("Occurrences"),
                             tr("Occurrences of the symbol under the cursor.\n"
                                "(Only the background will be applied.)"),
                             FormatDescription::ShowBackgroundControl);
    formatDescr.emplace_back(C_OCCURRENCES_UNUSED,
                             tr("Unused Occurrence"),
                             tr("Occurrences of unused variables."),
                             Qt::darkYellow,
                             QTextCharFormat::SingleUnderline);
    formatDescr.emplace_back(C_OCCURRENCES_RENAME, tr("Renaming Occurrence"),
                             tr("Occurrences of a symbol that will be renamed."),
                             FormatDescription::ShowBackgroundControl);

    // Standard categories
    formatDescr.emplace_back(C_NUMBER, tr("Number"), tr("Number literal."),
                             Qt::darkBlue);
    formatDescr.emplace_back(C_STRING, tr("String"),
                             tr("Character and string literals."), Qt::darkGreen);
    formatDescr.emplace_back(C_PRIMITIVE_TYPE, tr("Primitive Type"),
                             tr("Name of a primitive data type."), Qt::darkYellow);
    formatDescr.emplace_back(C_TYPE, tr("Type"), tr("Name of a type."),
                             Qt::darkMagenta);
    formatDescr.emplace_back(C_LOCAL, tr("Local"), tr("Local variables."));
    formatDescr.emplace_back(C_FIELD, tr("Field"),
                             tr("Class' data members."), Qt::darkRed);
    formatDescr.emplace_back(C_GLOBAL, tr("Global"), tr("Global variables."));
    formatDescr.emplace_back(C_ENUMERATION, tr("Enumeration"),
                             tr("Applied to enumeration items."), Qt::darkMagenta);

    Format functionFormat;
    formatDescr.emplace_back(C_FUNCTION, tr("Function"), tr("Name of a function."),
                             functionFormat);
    functionFormat.setItalic(true);
    formatDescr.emplace_back(C_VIRTUAL_METHOD, tr("Virtual Function"),
                             tr("Name of function declared as virtual."),
                             functionFormat);

    formatDescr.emplace_back(C_BINDING, tr("QML Binding"),
                             tr("QML item property, that allows a "
                                "binding to another property."),
                             Qt::darkRed);

    Format qmlLocalNameFormat;
    qmlLocalNameFormat.setItalic(true);
    formatDescr.emplace_back(C_QML_LOCAL_ID, tr("QML Local Id"),
                             tr("QML item id within a QML file."), qmlLocalNameFormat);
    formatDescr.emplace_back(C_QML_ROOT_OBJECT_PROPERTY,
                             tr("QML Root Object Property"),
                             tr("QML property of a parent item."), qmlLocalNameFormat);
    formatDescr.emplace_back(C_QML_SCOPE_OBJECT_PROPERTY,
                             tr("QML Scope Object Property"),
                             tr("Property of the same QML item."), qmlLocalNameFormat);
    formatDescr.emplace_back(C_QML_STATE_NAME, tr("QML State Name"),
                             tr("Name of a QML state."), qmlLocalNameFormat);

    formatDescr.emplace_back(C_QML_TYPE_ID, tr("QML Type Name"),
                             tr("Name of a QML type."), Qt::darkMagenta);

    Format qmlExternalNameFormat = qmlLocalNameFormat;
    qmlExternalNameFormat.setForeground(Qt::darkBlue);
    formatDescr.emplace_back(C_QML_EXTERNAL_ID, tr("QML External Id"),
                             tr("QML id defined in another QML file."),
                             qmlExternalNameFormat);
    formatDescr.emplace_back(C_QML_EXTERNAL_OBJECT_PROPERTY,
                             tr("QML External Object Property"),
                             tr("QML property defined in another QML file."),
                             qmlExternalNameFormat);

    Format jsLocalFormat;
    jsLocalFormat.setForeground(QColor(41, 133, 199)); // very light blue
    jsLocalFormat.setItalic(true);
    formatDescr.emplace_back(C_JS_SCOPE_VAR, tr("JavaScript Scope Var"),
                             tr("Variables defined inside the JavaScript file."),
                             jsLocalFormat);

    Format jsGlobalFormat;
    jsGlobalFormat.setForeground(QColor(0, 85, 175)); // light blue
    jsGlobalFormat.setItalic(true);
    formatDescr.emplace_back(C_JS_IMPORT_VAR, tr("JavaScript Import"),
                             tr("Name of a JavaScript import inside a QML file."),
                             jsGlobalFormat);
    formatDescr.emplace_back(C_JS_GLOBAL_VAR, tr("JavaScript Global Variable"),
                             tr("Variables defined outside the script."),
                             jsGlobalFormat);

    formatDescr.emplace_back(C_KEYWORD, tr("Keyword"),
                             tr("Reserved keywords of the programming language except "
                                "keywords denoting primitive types."), Qt::darkYellow);
    formatDescr.emplace_back(C_OPERATOR, tr("Operator"),
                             tr("Operators (for example operator++ or operator-=)."));
    formatDescr.emplace_back(C_PREPROCESSOR, tr("Preprocessor"),
                             tr("Preprocessor directives."), Qt::darkBlue);
    formatDescr.emplace_back(C_LABEL, tr("Label"), tr("Labels for goto statements."),
                             Qt::darkRed);
    formatDescr.emplace_back(C_COMMENT, tr("Comment"),
                             tr("All style of comments except Doxygen comments."),
                             Qt::darkGreen);
    formatDescr.emplace_back(C_DOXYGEN_COMMENT, tr("Doxygen Comment"),
                             tr("Doxygen comments."), Qt::darkBlue);
    formatDescr.emplace_back(C_DOXYGEN_TAG, tr("Doxygen Tag"), tr("Doxygen tags."),
                             Qt::blue);
    formatDescr.emplace_back(C_VISUAL_WHITESPACE, tr("Visual Whitespace"),
                             tr("Whitespace.\nWill not be applied to whitespace "
                                "in comments and strings."), Qt::lightGray);
    formatDescr.emplace_back(C_DISABLED_CODE, tr("Disabled Code"),
                             tr("Code disabled by preprocessor directives."));

    // Diff categories
    formatDescr.emplace_back(C_ADDED_LINE, tr("Added Line"),
                             tr("Applied to added lines in differences (in diff editor)."),
                             QColor(0, 170, 0));
    formatDescr.emplace_back(C_REMOVED_LINE, tr("Removed Line"),
                             tr("Applied to removed lines in differences (in diff editor)."),
                             Qt::red);
    formatDescr.emplace_back(C_DIFF_FILE, tr("Diff File"),
                             tr("Compared files (in diff editor)."), Qt::darkBlue);
    formatDescr.emplace_back(C_DIFF_LOCATION, tr("Diff Location"),
                             tr("Location in the files where the difference is "
                                "(in diff editor)."), Qt::blue);

    // New diff categories
    formatDescr.emplace_back(C_DIFF_FILE_LINE, tr("Diff File Line"),
                             tr("Applied to lines with file information "
                                "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(255, 255, 0)));
    formatDescr.emplace_back(C_DIFF_CONTEXT_LINE, tr("Diff Context Line"),
                             tr("Applied to lines describing hidden context "
                                "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(175, 215, 231)));
    formatDescr.emplace_back(C_DIFF_SOURCE_LINE, tr("Diff Source Line"),
                             tr("Applied to source lines with changes "
                                "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(255, 223, 223)));
    formatDescr.emplace_back(C_DIFF_SOURCE_CHAR, tr("Diff Source Character"),
                             tr("Applied to removed characters "
                                "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(255, 175, 175)));
    formatDescr.emplace_back(C_DIFF_DEST_LINE, tr("Diff Destination Line"),
                             tr("Applied to destination lines with changes "
                                "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(223, 255, 223)));
    formatDescr.emplace_back(C_DIFF_DEST_CHAR, tr("Diff Destination Character"),
                             tr("Applied to added characters "
                                "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(175, 255, 175)));

    formatDescr.emplace_back(C_LOG_CHANGE_LINE, tr("Log Change Line"),
                             tr("Applied to lines describing changes in VCS log."),
                             Format(QColor(192, 0, 0), QColor()));

    formatDescr.emplace_back(C_ERROR,
                             tr("Error"),
                             tr("Underline color of error diagnostics."),
                             QColor(255,0, 0),
                             QTextCharFormat::SingleUnderline,
                             FormatDescription::ShowUnderlineControl);
    formatDescr.emplace_back(C_ERROR_CONTEXT,
                             tr("Error Context"),
                             tr("Underline color of the contexts of error diagnostics."),
                             QColor(255,0, 0),
                             QTextCharFormat::DotLine,
                             FormatDescription::ShowUnderlineControl);
    formatDescr.emplace_back(C_WARNING,
                             tr("Warning"),
                             tr("Underline color of warning diagnostics."),
                             QColor(255, 190, 0),
                             QTextCharFormat::SingleUnderline,
                             FormatDescription::ShowUnderlineControl);
    formatDescr.emplace_back(C_WARNING_CONTEXT,
                             tr("Warning Context"),
                             tr("Underline color of the contexts of warning diagnostics."),
                             QColor(255, 190, 0),
                             QTextCharFormat::DotLine,
                             FormatDescription::ShowUnderlineControl);
    formatDescr.emplace_back(C_DECLARATION,
                             tr("Declaration"),
                             tr("Declaration of a function, variable, and so on."),
                             FormatDescription::ShowFontUnderlineAndRelativeControls);
    formatDescr.emplace_back(C_OUTPUT_ARGUMENT,
                             tr("Output Argument"),
                             tr("Writable arguments of a function call."),
                             FormatDescription::ShowFontUnderlineAndRelativeControls);

    d->m_fontSettingsPage = new FontSettingsPage(formatDescr,
                                                   Constants::TEXT_EDITOR_FONT_SETTINGS,
                                                   this);
    ExtensionSystem::PluginManager::addObject(d->m_fontSettingsPage);

    // Add the GUI used to configure the tab, storage and interaction settings
    BehaviorSettingsPageParameters behaviorSettingsPageParameters;
    behaviorSettingsPageParameters.id = Constants::TEXT_EDITOR_BEHAVIOR_SETTINGS;
    behaviorSettingsPageParameters.displayName = tr("Behavior");
    behaviorSettingsPageParameters.settingsPrefix = QLatin1String("text");
    d->m_behaviorSettingsPage = new BehaviorSettingsPage(behaviorSettingsPageParameters, this);
    ExtensionSystem::PluginManager::addObject(d->m_behaviorSettingsPage);

    DisplaySettingsPageParameters displaySettingsPageParameters;
    displaySettingsPageParameters.id = Constants::TEXT_EDITOR_DISPLAY_SETTINGS;
    displaySettingsPageParameters.displayName = tr("Display");
    displaySettingsPageParameters.settingsPrefix = QLatin1String("text");
    d->m_displaySettingsPage = new DisplaySettingsPage(displaySettingsPageParameters, this);
    ExtensionSystem::PluginManager::addObject(d->m_displaySettingsPage);

    d->m_highlighterSettingsPage =
        new HighlighterSettingsPage(Constants::TEXT_EDITOR_HIGHLIGHTER_SETTINGS, this);
    ExtensionSystem::PluginManager::addObject(d->m_highlighterSettingsPage);

    d->m_snippetsSettingsPage =
        new SnippetsSettingsPage(Constants::TEXT_EDITOR_SNIPPETS_SETTINGS, this);
    ExtensionSystem::PluginManager::addObject(d->m_snippetsSettingsPage);

    d->m_completionSettingsPage = new CompletionSettingsPage(this);
    ExtensionSystem::PluginManager::addObject(d->m_completionSettingsPage);

    connect(d->m_fontSettingsPage, &FontSettingsPage::changed,
            this, &TextEditorSettings::fontSettingsChanged);
    connect(d->m_behaviorSettingsPage, &BehaviorSettingsPage::typingSettingsChanged,
            this, &TextEditorSettings::typingSettingsChanged);
    connect(d->m_behaviorSettingsPage, &BehaviorSettingsPage::storageSettingsChanged,
            this, &TextEditorSettings::storageSettingsChanged);
    connect(d->m_behaviorSettingsPage, &BehaviorSettingsPage::behaviorSettingsChanged,
            this, &TextEditorSettings::behaviorSettingsChanged);
    connect(d->m_behaviorSettingsPage, &BehaviorSettingsPage::extraEncodingSettingsChanged,
            this, &TextEditorSettings::extraEncodingSettingsChanged);
    connect(d->m_displaySettingsPage, &DisplaySettingsPage::marginSettingsChanged,
            this, &TextEditorSettings::marginSettingsChanged);
    connect(d->m_displaySettingsPage, &DisplaySettingsPage::displaySettingsChanged,
            this, &TextEditorSettings::displaySettingsChanged);
    connect(d->m_completionSettingsPage, &CompletionSettingsPage::completionSettingsChanged,
            this, &TextEditorSettings::completionSettingsChanged);
    connect(d->m_completionSettingsPage, &CompletionSettingsPage::commentsSettingsChanged,
            this, &TextEditorSettings::commentsSettingsChanged);
}

TextEditorSettings::~TextEditorSettings()
{
    ExtensionSystem::PluginManager::removeObject(d->m_fontSettingsPage);
    ExtensionSystem::PluginManager::removeObject(d->m_behaviorSettingsPage);
    ExtensionSystem::PluginManager::removeObject(d->m_displaySettingsPage);
    ExtensionSystem::PluginManager::removeObject(d->m_highlighterSettingsPage);
    ExtensionSystem::PluginManager::removeObject(d->m_snippetsSettingsPage);
    ExtensionSystem::PluginManager::removeObject(d->m_completionSettingsPage);

    delete d;

    m_instance = 0;
}

TextEditorSettings *TextEditorSettings::instance()
{
    return m_instance;
}

const FontSettings &TextEditorSettings::fontSettings()
{
    return d->m_fontSettingsPage->fontSettings();
}

const TypingSettings &TextEditorSettings::typingSettings()
{
    return d->m_behaviorSettingsPage->typingSettings();
}

const StorageSettings &TextEditorSettings::storageSettings()
{
    return d->m_behaviorSettingsPage->storageSettings();
}

const BehaviorSettings &TextEditorSettings::behaviorSettings()
{
    return d->m_behaviorSettingsPage->behaviorSettings();
}

const MarginSettings &TextEditorSettings::marginSettings()
{
    return d->m_displaySettingsPage->marginSettings();
}

const DisplaySettings &TextEditorSettings::displaySettings()
{
    return d->m_displaySettingsPage->displaySettings();
}

const CompletionSettings &TextEditorSettings::completionSettings()
{
    return d->m_completionSettingsPage->completionSettings();
}

const HighlighterSettings &TextEditorSettings::highlighterSettings()
{
    return d->m_highlighterSettingsPage->highlighterSettings();
}

const ExtraEncodingSettings &TextEditorSettings::extraEncodingSettings()
{
    return d->m_behaviorSettingsPage->extraEncodingSettings();
}

const CommentsSettings &TextEditorSettings::commentsSettings()
{
    return d->m_completionSettingsPage->commentsSettings();
}

void TextEditorSettings::registerCodeStyleFactory(ICodeStylePreferencesFactory *factory)
{
    d->m_languageToFactory.insert(factory->languageId(), factory);
}

void TextEditorSettings::unregisterCodeStyleFactory(Core::Id languageId)
{
    d->m_languageToFactory.remove(languageId);
}

QMap<Core::Id, ICodeStylePreferencesFactory *> TextEditorSettings::codeStyleFactories()
{
    return d->m_languageToFactory;
}

ICodeStylePreferencesFactory *TextEditorSettings::codeStyleFactory(Core::Id languageId)
{
    return d->m_languageToFactory.value(languageId);
}

ICodeStylePreferences *TextEditorSettings::codeStyle()
{
    return d->m_behaviorSettingsPage->codeStyle();
}

ICodeStylePreferences *TextEditorSettings::codeStyle(Core::Id languageId)
{
    return d->m_languageToCodeStyle.value(languageId, codeStyle());
}

QMap<Core::Id, ICodeStylePreferences *> TextEditorSettings::codeStyles()
{
    return d->m_languageToCodeStyle;
}

void TextEditorSettings::registerCodeStyle(Core::Id languageId, ICodeStylePreferences *prefs)
{
    d->m_languageToCodeStyle.insert(languageId, prefs);
}

void TextEditorSettings::unregisterCodeStyle(Core::Id languageId)
{
    d->m_languageToCodeStyle.remove(languageId);
}

CodeStylePool *TextEditorSettings::codeStylePool()
{
    return d->m_behaviorSettingsPage->codeStylePool();
}

CodeStylePool *TextEditorSettings::codeStylePool(Core::Id languageId)
{
    return d->m_languageToCodeStylePool.value(languageId);
}

void TextEditorSettings::registerCodeStylePool(Core::Id languageId, CodeStylePool *pool)
{
    d->m_languageToCodeStylePool.insert(languageId, pool);
}

void TextEditorSettings::unregisterCodeStylePool(Core::Id languageId)
{
    d->m_languageToCodeStylePool.remove(languageId);
}

void TextEditorSettings::registerMimeTypeForLanguageId(const char *mimeType, Core::Id languageId)
{
    d->m_mimeTypeToLanguage.insert(QString::fromLatin1(mimeType), languageId);
}

Core::Id TextEditorSettings::languageId(const QString &mimeType)
{
    return d->m_mimeTypeToLanguage.value(mimeType);
}

int TextEditorSettings::increaseFontZoom(int step)
{
    FontSettings &fs = const_cast<FontSettings&>(d->m_fontSettingsPage->fontSettings());
    const int previousZoom = fs.fontZoom();
    const int newZoom = qMax(10, previousZoom + step);
    if (newZoom != previousZoom) {
        fs.setFontZoom(newZoom);
        d->m_fontSettingsPage->saveSettings();
    }
    return newZoom;
}

void TextEditorSettings::resetFontZoom()
{
    FontSettings &fs = const_cast<FontSettings&>(d->m_fontSettingsPage->fontSettings());
    fs.setFontZoom(100);
    d->m_fontSettingsPage->saveSettings();
}
