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

#include "behaviorsettings.h"
#include "behaviorsettingspage.h"
#include "completionsettings.h"
#include "completionsettingspage.h"
#include "displaysettings.h"
#include "displaysettingspage.h"
#include "extraencodingsettings.h"
#include "fontsettings.h"
#include "fontsettingspage.h"
#include "highlightersettingspage.h"
#include "icodestylepreferences.h"
#include "icodestylepreferencesfactory.h"
#include "marginsettings.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "texteditor.h"
#include "typingsettings.h"

#include <texteditor/snippets/snippetssettingspage.h>

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>

#include <QApplication>

using namespace TextEditor::Constants;
using namespace TextEditor::Internal;

namespace TextEditor {
namespace Internal {

class TextEditorSettingsPrivate
{
    Q_DECLARE_TR_FUNCTIONS(TextEditor::TextEditorSettings)

public:
    FontSettings m_fontSettings;
    FontSettingsPage m_fontSettingsPage{&m_fontSettings, initialFormats()};
    BehaviorSettingsPage m_behaviorSettingsPage;
    DisplaySettingsPage m_displaySettingsPage;
    HighlighterSettingsPage m_highlighterSettingsPage;
    SnippetsSettingsPage m_snippetsSettingsPage;
    CompletionSettingsPage m_completionSettingsPage;

    QMap<Utils::Id, ICodeStylePreferencesFactory *> m_languageToFactory;

    QMap<Utils::Id, ICodeStylePreferences *> m_languageToCodeStyle;
    QMap<Utils::Id, CodeStylePool *> m_languageToCodeStylePool;
    QMap<QString, Utils::Id> m_mimeTypeToLanguage;

private:
    static std::vector<FormatDescription> initialFormats();
};

FormatDescriptions TextEditorSettingsPrivate::initialFormats()
{
    // Add font preference page
    FormatDescriptions formatDescr;
    formatDescr.reserve(C_LAST_STYLE_SENTINEL);
    formatDescr.emplace_back(C_TEXT, tr("Text"),
                             tr("Generic text and punctuation tokens.\n"
                                                    "Applied to text that matched no other rule."),
                             Format{QColor{}, Qt::white});

    // Special categories
    const QPalette p = QApplication::palette();
    formatDescr.emplace_back(C_LINK, tr("Link"),
                             tr("Links that follow symbol under cursor."), Qt::blue);
    formatDescr.emplace_back(C_SELECTION, tr("Selection"), tr("Selected text."),
                             p.color(QPalette::HighlightedText));
    formatDescr.emplace_back(C_LINE_NUMBER, tr("Line Number"),
                             tr("Line numbers located on the left side of the editor."),
                             FormatDescription::ShowAllAbsoluteControlsExceptUnderline);
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
                                        FormatDescription::ShowAllAbsoluteControlsExceptUnderline);
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
    formatDescr.emplace_back(C_LOCAL, tr("Local"),
                             tr("Local variables."), QColor(9, 46, 100));
    formatDescr.emplace_back(C_FIELD, tr("Field"),
                             tr("Class' data members."), Qt::darkRed);
    formatDescr.emplace_back(C_GLOBAL, tr("Global"),
                             tr("Global variables."), QColor(206, 92, 0));
    formatDescr.emplace_back(C_ENUMERATION, tr("Enumeration"),
                             tr("Applied to enumeration items."), Qt::darkMagenta);

    Format functionFormat;
    functionFormat.setForeground(QColor(0, 103, 124));
    formatDescr.emplace_back(C_FUNCTION, tr("Function"), tr("Name of a function."),
                             functionFormat);
    Format declarationFormat;
    declarationFormat.setBold(true);
    formatDescr.emplace_back(C_DECLARATION,
                             tr("Function Declaration"),
                             tr("Style adjustments to (function) declarations."),
                             declarationFormat,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_FUNCTION_DEFINITION,
                             tr("Function Definition"),
                             tr("Name of function at its definition."),
                             FormatDescription::ShowAllControls);
    Format virtualFunctionFormat(functionFormat);
    virtualFunctionFormat.setItalic(true);
    formatDescr.emplace_back(C_VIRTUAL_METHOD, tr("Virtual Function"),
                             tr("Name of function declared as virtual."),
                             virtualFunctionFormat);

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
    formatDescr.emplace_back(C_PUNCTUATION, tr("Punctuation"),
                             tr("Punctuation excluding operators."));
    formatDescr.emplace_back(C_OPERATOR, tr("Operator"),
                             tr("Non user-defined language operators.\n"
                                "To style user-defined operators, use Overloaded Operator."),
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_OVERLOADED_OPERATOR,
                             tr("Overloaded Operators"),
                             tr("Calls and declarations of overloaded (user-defined) operators."),
                             functionFormat,
                             FormatDescription::ShowAllControls);
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
    formatDescr.emplace_back(C_LOG_AUTHOR_NAME, tr("Log Author Name"),
                             tr("Applied to author names in VCS log."),
                             Format(QColor("#007af4"), QColor()));
    formatDescr.emplace_back(C_LOG_COMMIT_DATE, tr("Log Commit Date"),
                             tr("Applied to commit dates in VCS log."),
                             Format(QColor("#006600"), QColor()));
    formatDescr.emplace_back(C_LOG_COMMIT_HASH, tr("Log Commit Hash"),
                             tr("Applied to commit hashes in VCS log."),
                             Format(QColor("#ff0000"), QColor()));
    formatDescr.emplace_back(C_LOG_DECORATION, tr("Log Decoration"),
                             tr("Applied to commit decorations in VCS log."),
                             Format(QColor("#ff00ff"), QColor()));
    formatDescr.emplace_back(C_LOG_COMMIT_SUBJECT, tr("Log Commit Subject"),
                             tr("Applied to commit subjects in VCS log."),
                             Format{QColor{}, QColor{}});

    // Mixin categories
    formatDescr.emplace_back(C_ERROR,
                             tr("Error"),
                             tr("Underline color of error diagnostics."),
                             QColor(255,0, 0),
                             QTextCharFormat::SingleUnderline,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_ERROR_CONTEXT,
                             tr("Error Context"),
                             tr("Underline color of the contexts of error diagnostics."),
                             QColor(255,0, 0),
                             QTextCharFormat::DotLine,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_WARNING,
                             tr("Warning"),
                             tr("Underline color of warning diagnostics."),
                             QColor(255, 190, 0),
                             QTextCharFormat::SingleUnderline,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_WARNING_CONTEXT,
                             tr("Warning Context"),
                             tr("Underline color of the contexts of warning diagnostics."),
                             QColor(255, 190, 0),
                             QTextCharFormat::DotLine,
                             FormatDescription::ShowAllControls);
    Format outputArgumentFormat;
    outputArgumentFormat.setItalic(true);
    formatDescr.emplace_back(C_OUTPUT_ARGUMENT,
                             tr("Output Argument"),
                             tr("Writable arguments of a function call."),
                             outputArgumentFormat,
                             FormatDescription::ShowAllControls);

    return formatDescr;
}

} // namespace Internal


static TextEditorSettingsPrivate *d = nullptr;
static TextEditorSettings *m_instance = nullptr;

TextEditorSettings::TextEditorSettings()
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;
    d = new Internal::TextEditorSettingsPrivate;

    // Note: default background colors are coming from FormatDescription::background()

    auto updateGeneralMessagesFontSettings = []() {
        Core::MessageManager::setFont(d->m_fontSettings.font());
    };
    connect(this, &TextEditorSettings::fontSettingsChanged,
            this, updateGeneralMessagesFontSettings);
    updateGeneralMessagesFontSettings();
    auto updateGeneralMessagesBehaviorSettings = []() {
        bool wheelZoom = d->m_behaviorSettingsPage.behaviorSettings().m_scrollWheelZooming;
        Core::MessageManager::setWheelZoomEnabled(wheelZoom);
    };
    connect(this, &TextEditorSettings::behaviorSettingsChanged,
            this, updateGeneralMessagesBehaviorSettings);
    updateGeneralMessagesBehaviorSettings();

    auto updateCamelCaseNavigation = [] {
        Utils::FancyLineEdit::setCamelCaseNavigationEnabled(behaviorSettings().m_camelCaseNavigation);
    };
    connect(this, &TextEditorSettings::behaviorSettingsChanged,
            this, updateCamelCaseNavigation);
    updateCamelCaseNavigation();
}

TextEditorSettings::~TextEditorSettings()
{
    delete d;

    m_instance = nullptr;
}

TextEditorSettings *TextEditorSettings::instance()
{
    return m_instance;
}

const FontSettings &TextEditorSettings::fontSettings()
{
    return d->m_fontSettings;
}

const TypingSettings &TextEditorSettings::typingSettings()
{
    return d->m_behaviorSettingsPage.typingSettings();
}

const StorageSettings &TextEditorSettings::storageSettings()
{
    return d->m_behaviorSettingsPage.storageSettings();
}

const BehaviorSettings &TextEditorSettings::behaviorSettings()
{
    return d->m_behaviorSettingsPage.behaviorSettings();
}

const MarginSettings &TextEditorSettings::marginSettings()
{
    return d->m_displaySettingsPage.marginSettings();
}

const DisplaySettings &TextEditorSettings::displaySettings()
{
    return d->m_displaySettingsPage.displaySettings();
}

const CompletionSettings &TextEditorSettings::completionSettings()
{
    return d->m_completionSettingsPage.completionSettings();
}

const HighlighterSettings &TextEditorSettings::highlighterSettings()
{
    return d->m_highlighterSettingsPage.highlighterSettings();
}

const ExtraEncodingSettings &TextEditorSettings::extraEncodingSettings()
{
    return d->m_behaviorSettingsPage.extraEncodingSettings();
}

const CommentsSettings &TextEditorSettings::commentsSettings()
{
    return d->m_completionSettingsPage.commentsSettings();
}

void TextEditorSettings::registerCodeStyleFactory(ICodeStylePreferencesFactory *factory)
{
    d->m_languageToFactory.insert(factory->languageId(), factory);
}

void TextEditorSettings::unregisterCodeStyleFactory(Utils::Id languageId)
{
    d->m_languageToFactory.remove(languageId);
}

const QMap<Utils::Id, ICodeStylePreferencesFactory *> &TextEditorSettings::codeStyleFactories()
{
    return d->m_languageToFactory;
}

ICodeStylePreferencesFactory *TextEditorSettings::codeStyleFactory(Utils::Id languageId)
{
    return d->m_languageToFactory.value(languageId);
}

ICodeStylePreferences *TextEditorSettings::codeStyle()
{
    return d->m_behaviorSettingsPage.codeStyle();
}

ICodeStylePreferences *TextEditorSettings::codeStyle(Utils::Id languageId)
{
    return d->m_languageToCodeStyle.value(languageId, codeStyle());
}

QMap<Utils::Id, ICodeStylePreferences *> TextEditorSettings::codeStyles()
{
    return d->m_languageToCodeStyle;
}

void TextEditorSettings::registerCodeStyle(Utils::Id languageId, ICodeStylePreferences *prefs)
{
    d->m_languageToCodeStyle.insert(languageId, prefs);
}

void TextEditorSettings::unregisterCodeStyle(Utils::Id languageId)
{
    d->m_languageToCodeStyle.remove(languageId);
}

CodeStylePool *TextEditorSettings::codeStylePool()
{
    return d->m_behaviorSettingsPage.codeStylePool();
}

CodeStylePool *TextEditorSettings::codeStylePool(Utils::Id languageId)
{
    return d->m_languageToCodeStylePool.value(languageId);
}

void TextEditorSettings::registerCodeStylePool(Utils::Id languageId, CodeStylePool *pool)
{
    d->m_languageToCodeStylePool.insert(languageId, pool);
}

void TextEditorSettings::unregisterCodeStylePool(Utils::Id languageId)
{
    d->m_languageToCodeStylePool.remove(languageId);
}

void TextEditorSettings::registerMimeTypeForLanguageId(const char *mimeType, Utils::Id languageId)
{
    d->m_mimeTypeToLanguage.insert(QString::fromLatin1(mimeType), languageId);
}

Utils::Id TextEditorSettings::languageId(const QString &mimeType)
{
    return d->m_mimeTypeToLanguage.value(mimeType);
}

static void setFontZoom(int zoom)
{
    d->m_fontSettingsPage.setFontZoom(zoom);
    d->m_fontSettings.setFontZoom(zoom);
    d->m_fontSettings.toSettings(Core::ICore::settings());
    emit m_instance->fontSettingsChanged(d->m_fontSettings);
}

int TextEditorSettings::increaseFontZoom(int step)
{
    const int previousZoom = d->m_fontSettings.fontZoom();
    const int newZoom = qMax(10, previousZoom + step);
    if (newZoom != previousZoom)
        setFontZoom(newZoom);
    return newZoom;
}

void TextEditorSettings::resetFontZoom()
{
    setFontZoom(100);
}

} // TextEditor
