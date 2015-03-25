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

    QMap<Core::Id, ICodeStylePreferencesFactory *> m_languageToFactory;

    QMap<Core::Id, ICodeStylePreferences *> m_languageToCodeStyle;
    QMap<Core::Id, CodeStylePool *> m_languageToCodeStylePool;
    QMap<QString, Core::Id> m_mimeTypeToLanguage;

    CompletionSettings m_completionSettings;
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
    formatDescr.append(FormatDescription(C_PRIMITIVE_TYPE, tr("Primitive Type"),
                                         tr("Name of a primitive data type."), Qt::darkYellow));
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
    formatDescr.append(FormatDescription(C_VIRTUAL_METHOD, tr("Virtual Function"),
                                         tr("Name of function declared as virtual."),
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
                                         tr("Reserved keywords of the programming language except "
                                            "keywords denoting primitive types."), Qt::darkYellow));
    formatDescr.append(FormatDescription(C_OPERATOR, tr("Operator"),
                                         tr("Operators (for example operator++ or operator-=).")));
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
                                         tr("Whitespace.\nWill not be applied to whitespace "
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

    // New diff categories
    formatDescr.append(FormatDescription(C_DIFF_FILE_LINE, tr("Diff File Line"),
                                         tr("Applied to lines with file information "
                                            "in differences (in side-by-side diff editor)."),
                                         Format(QColor(), QColor(255, 255, 0))));
    formatDescr.append(FormatDescription(C_DIFF_CONTEXT_LINE, tr("Diff Context Line"),
                                         tr("Applied to lines describing hidden context "
                                            "in differences (in side-by-side diff editor)."),
                                         Format(QColor(), QColor(175, 215, 231))));
    formatDescr.append(FormatDescription(C_DIFF_SOURCE_LINE, tr("Diff Source Line"),
                                         tr("Applied to source lines with changes "
                                            "in differences (in side-by-side diff editor)."),
                                         Format(QColor(), QColor(255, 223, 223))));
    formatDescr.append(FormatDescription(C_DIFF_SOURCE_CHAR, tr("Diff Source Character"),
                                         tr("Applied to removed characters "
                                            "in differences (in side-by-side diff editor)."),
                                         Format(QColor(), QColor(255, 175, 175))));
    formatDescr.append(FormatDescription(C_DIFF_DEST_LINE, tr("Diff Destination Line"),
                                         tr("Applied to destination lines with changes "
                                            "in differences (in side-by-side diff editor)."),
                                         Format(QColor(), QColor(223, 255, 223))));
    formatDescr.append(FormatDescription(C_DIFF_DEST_CHAR, tr("Diff Destination Character"),
                                         tr("Applied to added characters "
                                            "in differences (in side-by-side diff editor)."),
                                         Format(QColor(), QColor(175, 255, 175))));

    formatDescr.append(FormatDescription(C_LOG_CHANGE_LINE, tr("Log Change Line"),
                                         tr("Applied to lines describing changes in VCS log"),
                                         Format(QColor(192, 0, 0), QColor())));

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

    connect(d->m_fontSettingsPage, SIGNAL(changed(TextEditor::FontSettings)),
            this, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)));
    connect(d->m_behaviorSettingsPage, SIGNAL(typingSettingsChanged(TextEditor::TypingSettings)),
            this, SIGNAL(typingSettingsChanged(TextEditor::TypingSettings)));
    connect(d->m_behaviorSettingsPage, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)),
            this, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)));
    connect(d->m_behaviorSettingsPage, SIGNAL(behaviorSettingsChanged(TextEditor::BehaviorSettings)),
            this, SIGNAL(behaviorSettingsChanged(TextEditor::BehaviorSettings)));
    connect(d->m_behaviorSettingsPage, SIGNAL(extraEncodingSettingsChanged(TextEditor::ExtraEncodingSettings)),
            this, SIGNAL(extraEncodingSettingsChanged(TextEditor::ExtraEncodingSettings)));
    connect(d->m_displaySettingsPage, SIGNAL(marginSettingsChanged(TextEditor::MarginSettings)),
            this, SIGNAL(marginSettingsChanged(TextEditor::MarginSettings)));
    connect(d->m_displaySettingsPage, SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)),
            this, SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)));

    // TODO: Move these settings to TextEditor category
    d->m_completionSettings.fromSettings(QLatin1String("CppTools/"), Core::ICore::settings());
}

TextEditorSettings::~TextEditorSettings()
{
    ExtensionSystem::PluginManager::removeObject(d->m_fontSettingsPage);
    ExtensionSystem::PluginManager::removeObject(d->m_behaviorSettingsPage);
    ExtensionSystem::PluginManager::removeObject(d->m_displaySettingsPage);
    ExtensionSystem::PluginManager::removeObject(d->m_highlighterSettingsPage);
    ExtensionSystem::PluginManager::removeObject(d->m_snippetsSettingsPage);

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
    return d->m_completionSettings;
}

const HighlighterSettings &TextEditorSettings::highlighterSettings()
{
    return d->m_highlighterSettingsPage->highlighterSettings();
}

const ExtraEncodingSettings &TextEditorSettings::extraEncodingSettings()
{
    return d->m_behaviorSettingsPage->extraEncodingSettings();
}

void TextEditorSettings::setCompletionSettings(const CompletionSettings &settings)
{
    if (d->m_completionSettings == settings)
        return;

    d->m_completionSettings = settings;
    d->m_completionSettings.toSettings(QLatin1String("CppTools/"), Core::ICore::settings());

    emit m_instance->completionSettingsChanged(d->m_completionSettings);
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

void TextEditorSettings::fontZoomRequested(int zoom)
{
    FontSettings &fs = const_cast<FontSettings&>(d->m_fontSettingsPage->fontSettings());
    fs.setFontZoom(qMax(10, fs.fontZoom() + zoom));
    d->m_fontSettingsPage->saveSettings();
}

void TextEditorSettings::zoomResetRequested()
{
    FontSettings &fs = const_cast<FontSettings&>(d->m_fontSettingsPage->fontSettings());
    fs.setFontZoom(100);
    d->m_fontSettingsPage->saveSettings();
}
