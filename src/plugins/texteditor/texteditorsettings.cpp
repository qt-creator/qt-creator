// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texteditorsettings.h"

#include "behaviorsettings.h"
#include "behaviorsettingspage.h"
#include "commentssettings.h"
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
#include "texteditortr.h"
#include "typingsettings.h"
#include "snippets/snippetssettingspage.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>

#include <QApplication>

using namespace TextEditor::Constants;
using namespace TextEditor::Internal;

namespace TextEditor {
namespace Internal {

class TextEditorSettingsPrivate
{
public:
    FontSettings m_fontSettings;
    FontSettingsPage m_fontSettingsPage{&m_fontSettings, initialFormats()};
    BehaviorSettingsPage m_behaviorSettingsPage;
    DisplaySettingsPage m_displaySettingsPage;
    HighlighterSettingsPage m_highlighterSettingsPage;
    SnippetsSettingsPage m_snippetsSettingsPage;
    CompletionSettingsPage m_completionSettingsPage;
    CommentsSettingsPage m_commentsSettingsPage;

    QMap<Utils::Id, ICodeStylePreferencesFactory *> m_languageToFactory;

    QMap<Utils::Id, ICodeStylePreferences *> m_languageToCodeStyle;
    QMap<Utils::Id, CodeStylePool *> m_languageToCodeStylePool;
    QMap<QString, Utils::Id> m_mimeTypeToLanguage;

    std::function<CommentsSettings::Data(const Utils::FilePath &)> m_retrieveCommentsSettings;

private:
    static std::vector<FormatDescription> initialFormats();
};

FormatDescriptions TextEditorSettingsPrivate::initialFormats()
{
    // Add font preference page
    FormatDescriptions formatDescr;
    formatDescr.reserve(C_LAST_STYLE_SENTINEL);
    formatDescr.emplace_back(C_TEXT, Tr::tr("Text"),
                             Tr::tr("Generic text and punctuation tokens.\n"
                                    "Applied to text that matched no other rule."),
                             Format{Qt::black, Qt::white});

    // Special categories
    const QPalette p = QApplication::palette();
    formatDescr.emplace_back(C_LINK, Tr::tr("Link"),
                             Tr::tr("Links that follow symbol under cursor."), Qt::blue);
    formatDescr.emplace_back(C_SELECTION, Tr::tr("Selection"), Tr::tr("Selected text."),
                             p.color(QPalette::HighlightedText));
    formatDescr.emplace_back(C_LINE_NUMBER, Tr::tr("Line Number"),
                             Tr::tr("Line numbers located on the left side of the editor."),
                             FormatDescription::ShowAllAbsoluteControlsExceptUnderline);
    formatDescr.emplace_back(C_SEARCH_RESULT, Tr::tr("Search Result"),
                             Tr::tr("Highlighted search results inside the editor."),
                             FormatDescription::ShowBackgroundControl);
    formatDescr.emplace_back(C_SEARCH_RESULT_ALT1, Tr::tr("Search Result (Alternative 1)"),
                             Tr::tr("Highlighted search results inside the editor.\n"
                                    "Used to mark read accesses to C++ symbols."),
                             FormatDescription::ShowBackgroundControl);
    formatDescr.emplace_back(C_SEARCH_RESULT_ALT2, Tr::tr("Search Result (Alternative 2)"),
                             Tr::tr("Highlighted search results inside the editor.\n"
                                    "Used to mark write accesses to C++ symbols."),
                             FormatDescription::ShowBackgroundControl);
    formatDescr.emplace_back(C_SEARCH_RESULT_CONTAINING_FUNCTION,
                             Tr::tr("Search Result Containing function"),
                             Tr::tr("Highlighted search results inside the editor.\n"
                                    "Used to mark containing function of the symbol usage."),
                             FormatDescription::ShowForeAndBackgroundControl);
    formatDescr.emplace_back(C_SEARCH_SCOPE, Tr::tr("Search Scope"),
                             Tr::tr("Section where the pattern is searched in."),
                             FormatDescription::ShowBackgroundControl);
    formatDescr.emplace_back(C_PARENTHESES, Tr::tr("Parentheses"),
                             Tr::tr("Displayed when matching parentheses, square brackets "
                                    "or curly brackets are found."));
    formatDescr.emplace_back(C_PARENTHESES_MISMATCH, Tr::tr("Mismatched Parentheses"),
                             Tr::tr("Displayed when mismatched parentheses, "
                                    "square brackets, or curly brackets are found."));
    formatDescr.emplace_back(C_AUTOCOMPLETE, Tr::tr("Auto Complete"),
                             Tr::tr("Displayed when a character is automatically inserted "
                                    "like brackets or quotes."));
    formatDescr.emplace_back(C_CURRENT_LINE, Tr::tr("Current Line"),
                             Tr::tr("Line where the cursor is placed in."),
                             FormatDescription::ShowBackgroundControl);

    FormatDescription currentLineNumber(C_CURRENT_LINE_NUMBER,
                                        Tr::tr("Current Line Number"),
                                        Tr::tr("Line number located on the left side of the "
                                               "editor where the cursor is placed in."),
                                        Qt::darkGray,
                                        FormatDescription::ShowAllAbsoluteControlsExceptUnderline);
    currentLineNumber.format().setBold(true);
    formatDescr.push_back(std::move(currentLineNumber));


    formatDescr.emplace_back(C_OCCURRENCES, Tr::tr("Occurrences"),
                             Tr::tr("Occurrences of the symbol under the cursor.\n"
                                    "(Only the background will be applied.)"),
                             FormatDescription::ShowBackgroundControl);
    formatDescr.emplace_back(C_OCCURRENCES_UNUSED,
                             Tr::tr("Unused Occurrence"),
                             Tr::tr("Occurrences of unused variables."),
                             Qt::darkYellow,
                             QTextCharFormat::SingleUnderline);
    formatDescr.emplace_back(C_OCCURRENCES_RENAME, Tr::tr("Renaming Occurrence"),
                             Tr::tr("Occurrences of a symbol that will be renamed."),
                             FormatDescription::ShowBackgroundControl);

    // Standard categories
    formatDescr.emplace_back(C_NUMBER, Tr::tr("Number"), Tr::tr("Number literal."),
                             Qt::darkBlue);
    formatDescr.emplace_back(C_STRING, Tr::tr("String"),
                             Tr::tr("Character and string literals."), Qt::darkGreen);
    formatDescr.emplace_back(C_PRIMITIVE_TYPE, Tr::tr("Primitive Type"),
                             Tr::tr("Name of a primitive data type."), Qt::darkYellow);
    formatDescr.emplace_back(C_TYPE, Tr::tr("Type"), Tr::tr("Name of a type."),
                             Qt::darkMagenta);
    formatDescr.emplace_back(C_CONCEPT, Tr::tr("Concept"), Tr::tr("Name of a concept."),
                             Qt::darkMagenta);
    formatDescr.emplace_back(C_NAMESPACE, Tr::tr("Namespace"), Tr::tr("Name of a namespace."),
                             Qt::darkGreen);
    formatDescr.emplace_back(C_LOCAL, Tr::tr("Local"),
                             Tr::tr("Local variables."), QColor(9, 46, 100));
    formatDescr.emplace_back(C_PARAMETER, Tr::tr("Parameter"),
                             Tr::tr("Function or method parameters."), QColor(9, 46, 100));
    formatDescr.emplace_back(C_FIELD, Tr::tr("Field"),
                             Tr::tr("Class' data members."), Qt::darkRed);
    formatDescr.emplace_back(C_GLOBAL, Tr::tr("Global"),
                             Tr::tr("Global variables."), QColor(206, 92, 0));
    formatDescr.emplace_back(C_ENUMERATION, Tr::tr("Enumeration"),
                             Tr::tr("Applied to enumeration items."), Qt::darkMagenta);

    Format functionFormat;
    functionFormat.setForeground(QColor(0, 103, 124));
    formatDescr.emplace_back(C_FUNCTION, Tr::tr("Function"), Tr::tr("Name of a function."),
                             functionFormat);
    Format declarationFormat;
    declarationFormat.setBold(true);
    formatDescr.emplace_back(C_DECLARATION,
                             Tr::tr("Declaration"),
                             Tr::tr("Style adjustments to declarations."),
                             declarationFormat,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_FUNCTION_DEFINITION,
                             Tr::tr("Function Definition"),
                             Tr::tr("Name of function at its definition."),
                             FormatDescription::ShowAllControls);
    Format virtualFunctionFormat(functionFormat);
    virtualFunctionFormat.setItalic(true);
    formatDescr.emplace_back(C_VIRTUAL_METHOD, Tr::tr("Virtual Function"),
                             Tr::tr("Name of function declared as virtual."),
                             virtualFunctionFormat);

    formatDescr.emplace_back(C_BINDING, Tr::tr("QML Binding"),
                             Tr::tr("QML item property, that allows a "
                                    "binding to another property."),
                             Qt::darkRed);

    Format qmlLocalNameFormat;
    qmlLocalNameFormat.setItalic(true);
    formatDescr.emplace_back(C_QML_LOCAL_ID, Tr::tr("QML Local Id"),
                             Tr::tr("QML item id within a QML file."), qmlLocalNameFormat);
    formatDescr.emplace_back(C_QML_ROOT_OBJECT_PROPERTY,
                             Tr::tr("QML Root Object Property"),
                             Tr::tr("QML property of a parent item."), qmlLocalNameFormat);
    formatDescr.emplace_back(C_QML_SCOPE_OBJECT_PROPERTY,
                             Tr::tr("QML Scope Object Property"),
                             Tr::tr("Property of the same QML item."), qmlLocalNameFormat);
    formatDescr.emplace_back(C_QML_STATE_NAME, Tr::tr("QML State Name"),
                             Tr::tr("Name of a QML state."), qmlLocalNameFormat);

    formatDescr.emplace_back(C_QML_TYPE_ID, Tr::tr("QML Type Name"),
                             Tr::tr("Name of a QML type."), Qt::darkMagenta);

    Format qmlExternalNameFormat = qmlLocalNameFormat;
    qmlExternalNameFormat.setForeground(Qt::darkBlue);
    formatDescr.emplace_back(C_QML_EXTERNAL_ID, Tr::tr("QML External Id"),
                             Tr::tr("QML id defined in another QML file."),
                             qmlExternalNameFormat);
    formatDescr.emplace_back(C_QML_EXTERNAL_OBJECT_PROPERTY,
                             Tr::tr("QML External Object Property"),
                             Tr::tr("QML property defined in another QML file."),
                             qmlExternalNameFormat);

    Format jsLocalFormat;
    jsLocalFormat.setForeground(QColor(41, 133, 199)); // very light blue
    jsLocalFormat.setItalic(true);
    formatDescr.emplace_back(C_JS_SCOPE_VAR, Tr::tr("JavaScript Scope Var"),
                             Tr::tr("Variables defined inside the JavaScript file."),
                             jsLocalFormat);

    Format jsGlobalFormat;
    jsGlobalFormat.setForeground(QColor(0, 85, 175)); // light blue
    jsGlobalFormat.setItalic(true);
    formatDescr.emplace_back(C_JS_IMPORT_VAR, Tr::tr("JavaScript Import"),
                             Tr::tr("Name of a JavaScript import inside a QML file."),
                             jsGlobalFormat);
    formatDescr.emplace_back(C_JS_GLOBAL_VAR, Tr::tr("JavaScript Global Variable"),
                             Tr::tr("Variables defined outside the script."),
                             jsGlobalFormat);

    formatDescr.emplace_back(C_KEYWORD, Tr::tr("Keyword"),
                             Tr::tr("Reserved keywords of the programming language except "
                                "keywords denoting primitive types."), Qt::darkYellow);
    formatDescr.emplace_back(C_PUNCTUATION, Tr::tr("Punctuation"),
                             Tr::tr("Punctuation excluding operators."));
    formatDescr.emplace_back(C_OPERATOR, Tr::tr("Operator"),
                             Tr::tr("Non user-defined language operators.\n"
                                    "To style user-defined operators, use Overloaded Operator."),
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_OVERLOADED_OPERATOR,
                             Tr::tr("Overloaded Operators"),
                             Tr::tr("Calls and declarations of overloaded (user-defined) operators."),
                             functionFormat,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_PREPROCESSOR, Tr::tr("Preprocessor"),
                             Tr::tr("Preprocessor directives."), Qt::darkBlue);
    formatDescr.emplace_back(C_MACRO, Tr::tr("Macro"),
                             Tr::tr("Macros."), functionFormat);
    formatDescr.emplace_back(C_LABEL, Tr::tr("Label"), Tr::tr("Labels for goto statements."),
                             Qt::darkRed);
    formatDescr.emplace_back(C_COMMENT, Tr::tr("Comment"),
                             Tr::tr("All style of comments except Doxygen comments."),
                             Qt::darkGreen);
    formatDescr.emplace_back(C_DOXYGEN_COMMENT, Tr::tr("Doxygen Comment"),
                             Tr::tr("Doxygen comments."), Qt::darkBlue);
    formatDescr.emplace_back(C_DOXYGEN_TAG, Tr::tr("Doxygen Tag"), Tr::tr("Doxygen tags."),
                             Qt::blue);
    formatDescr.emplace_back(C_VISUAL_WHITESPACE, Tr::tr("Visual Whitespace"),
                             Tr::tr("Whitespace.\nWill not be applied to whitespace "
                                    "in comments and strings."), Qt::lightGray);
    formatDescr.emplace_back(C_DISABLED_CODE, Tr::tr("Disabled Code"),
                             Tr::tr("Code disabled by preprocessor directives."));

    // Diff categories
    formatDescr.emplace_back(C_ADDED_LINE, Tr::tr("Added Line"),
                             Tr::tr("Applied to added lines in differences (in diff editor)."),
                             QColor(0, 170, 0));
    formatDescr.emplace_back(C_REMOVED_LINE, Tr::tr("Removed Line"),
                             Tr::tr("Applied to removed lines in differences (in diff editor)."),
                             Qt::red);
    formatDescr.emplace_back(C_DIFF_FILE, Tr::tr("Diff File"),
                             Tr::tr("Compared files (in diff editor)."), Qt::darkBlue);
    formatDescr.emplace_back(C_DIFF_LOCATION, Tr::tr("Diff Location"),
                             Tr::tr("Location in the files where the difference is "
                                    "(in diff editor)."), Qt::blue);

    // New diff categories
    formatDescr.emplace_back(C_DIFF_FILE_LINE, Tr::tr("Diff File Line"),
                             Tr::tr("Applied to lines with file information "
                                    "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(255, 255, 0)));
    formatDescr.emplace_back(C_DIFF_CONTEXT_LINE, Tr::tr("Diff Context Line"),
                             Tr::tr("Applied to lines describing hidden context "
                                    "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(175, 215, 231)));
    formatDescr.emplace_back(C_DIFF_SOURCE_LINE, Tr::tr("Diff Source Line"),
                             Tr::tr("Applied to source lines with changes "
                                    "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(255, 223, 223)));
    formatDescr.emplace_back(C_DIFF_SOURCE_CHAR, Tr::tr("Diff Source Character"),
                             Tr::tr("Applied to removed characters "
                                    "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(255, 175, 175)));
    formatDescr.emplace_back(C_DIFF_DEST_LINE, Tr::tr("Diff Destination Line"),
                             Tr::tr("Applied to destination lines with changes "
                                    "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(223, 255, 223)));
    formatDescr.emplace_back(C_DIFF_DEST_CHAR, Tr::tr("Diff Destination Character"),
                             Tr::tr("Applied to added characters "
                                    "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(175, 255, 175)));

    formatDescr.emplace_back(C_LOG_CHANGE_LINE, Tr::tr("Log Change Line"),
                             Tr::tr("Applied to lines describing changes in VCS log."),
                             Format(QColor(192, 0, 0), QColor()));
    formatDescr.emplace_back(C_LOG_AUTHOR_NAME, Tr::tr("Log Author Name"),
                             Tr::tr("Applied to author names in VCS log."),
                             Format(QColor(0x007af4), QColor()));
    formatDescr.emplace_back(C_LOG_COMMIT_DATE, Tr::tr("Log Commit Date"),
                             Tr::tr("Applied to commit dates in VCS log."),
                             Format(QColor(0x006600), QColor()));
    formatDescr.emplace_back(C_LOG_COMMIT_HASH, Tr::tr("Log Commit Hash"),
                             Tr::tr("Applied to commit hashes in VCS log."),
                             Format(QColor(0xff0000), QColor()));
    formatDescr.emplace_back(C_LOG_DECORATION, Tr::tr("Log Decoration"),
                             Tr::tr("Applied to commit decorations in VCS log."),
                             Format(QColor(0xff00ff), QColor()));
    formatDescr.emplace_back(C_LOG_COMMIT_SUBJECT, Tr::tr("Log Commit Subject"),
                             Tr::tr("Applied to commit subjects in VCS log."),
                             Format{QColor{}, QColor{}});

    // Mixin categories
    formatDescr.emplace_back(C_ERROR,
                             Tr::tr("Error"),
                             Tr::tr("Underline color of error diagnostics."),
                             QColor(255,0, 0),
                             QTextCharFormat::SingleUnderline,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_ERROR_CONTEXT,
                             Tr::tr("Error Context"),
                             Tr::tr("Underline color of the contexts of error diagnostics."),
                             QColor(255,0, 0),
                             QTextCharFormat::DotLine,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_WARNING,
                             Tr::tr("Warning"),
                             Tr::tr("Underline color of warning diagnostics."),
                             QColor(255, 190, 0),
                             QTextCharFormat::SingleUnderline,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_WARNING_CONTEXT,
                             Tr::tr("Warning Context"),
                             Tr::tr("Underline color of the contexts of warning diagnostics."),
                             QColor(255, 190, 0),
                             QTextCharFormat::DotLine,
                             FormatDescription::ShowAllControls);
    Format outputArgumentFormat;
    outputArgumentFormat.setItalic(true);
    formatDescr.emplace_back(C_OUTPUT_ARGUMENT,
                             Tr::tr("Output Argument"),
                             Tr::tr("Writable arguments of a function call."),
                             outputArgumentFormat,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_STATIC_MEMBER,
                             Tr::tr("Static Member"),
                             Tr::tr("Names of static fields or member functions."),
                             FormatDescription::ShowAllControls);

    const auto cocoControls = FormatDescription::ShowControls(
        FormatDescription::ShowAllAbsoluteControls | FormatDescription::ShowRelativeControls);
    formatDescr.emplace_back(C_COCO_CODE_ADDED,
                             Tr::tr("Code Coverage Added Code"),
                             Tr::tr("New code that was not checked for tests."),
                             cocoControls);
    formatDescr.emplace_back(C_COCO_PARTIALLY_COVERED,
                             Tr::tr("Partially Covered Code"),
                             Tr::tr("Partial branch/condition coverage."),
                             Qt::darkYellow,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_NOT_COVERED,
                             Tr::tr("Uncovered Code"),
                             Tr::tr("Not covered at all."),
                             Qt::red,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_FULLY_COVERED,
                             Tr::tr("Fully Covered Code"),
                             Tr::tr("Fully covered code."),
                             Qt::green,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_MANUALLY_VALIDATED,
                             Tr::tr("Manually Validated Code"),
                             Tr::tr("User added validation."),
                             Qt::blue,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_DEAD_CODE,
                             Tr::tr("Code Coverage Dead Code"),
                             Tr::tr("Unreachable code."),
                             Qt::magenta,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_EXECUTION_COUNT_TOO_LOW,
                             Tr::tr("Code Coverage Execution Count Too Low"),
                             Tr::tr("Minimum count not reached."),
                             Qt::red,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_NOT_COVERED_INFO,
                             Tr::tr("Implicitly Not Covered Code"),
                             Tr::tr("PLACEHOLDER"),
                             Qt::red,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_COVERED_INFO,
                             Tr::tr("Implicitly Covered Code"),
                             Tr::tr("PLACEHOLDER"),
                             Qt::green,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_MANUALLY_VALIDATED_INFO,
                             Tr::tr("Implicit Manual Coverage Validation"),
                             Tr::tr("PLACEHOLDER"),
                             Qt::blue,
                             cocoControls);

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

void TextEditorSettings::setCommentsSettingsRetriever(
    const std::function<CommentsSettings::Data(const Utils::FilePath &)> &retrieve)
{
    d->m_retrieveCommentsSettings = retrieve;
}

CommentsSettings::Data TextEditorSettings::commentsSettings(const Utils::FilePath &filePath)
{
    QTC_ASSERT(d->m_retrieveCommentsSettings, return CommentsSettings::data());
    return d->m_retrieveCommentsSettings(filePath);
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
