/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_THEME_H
#define KSYNTAXHIGHLIGHTING_THEME_H

#include "ksyntaxhighlighting_export.h"

#include <QColor>
#include <QExplicitlySharedDataPointer>
#include <QTypeInfo>
#include <qobjectdefs.h>

namespace KSyntaxHighlighting
{
class ThemeData;
class RepositoryPrivate;

/*!
 * \class KSyntaxHighlighting::Theme
 * \inheaderfile KSyntaxHighlighting/Theme
 * \inmodule KSyntaxHighlighting
 *
 * \brief Color theme definition used for highlighting.
 *
 * The Theme provides a full color theme for painting the highlighted text.
 * One Theme is defined either as a *.theme file on disk, or as a file compiled
 * into the SyntaxHighlighting library by using Qt's resource system. Each
 * Theme has a unique name(), including a translatedName() if put into the UI.
 * Themes shipped by default are typically read-only, see isReadOnly().
 *
 * A Theme defines two sets of colors:
 * \list
 * \li Text colors, including foreground and background colors, colors for
 *   selected text, and properties such as bold and italic. These colors are
 *   used e.g. by the SyntaxHighlighter.
 * \li Editor colors, including a background color for the entire editor widget,
 *   the line number color, code folding colors, etc.
 * \endlist
 *
 * \section1 Text Colors and the Class Format
 *
 * The text colors are used for syntax highlighting.
 * // TODO: elaborate more and explain relation to Format class
 *
 * \section1 theme_editor_colors Editor Colors
 *
 * If you want to use the SyntaxHighlighting framework to write your own text
 * editor, you also need to paint the background of the editing widget. In
 * addition, the editor may support showing line numbers, a folding bar, a
 * highlight for the current text line, and similar features. All these colors
 * are defined in terms of the "editor colors" and accessible by calling
 * editorColor() with the desired enum EditorColorRole.
 *
 * \section1 Accessing a Theme
 *
 * All available Theme%s are accessed through the Repository. These themes are
 * typically valid themes. If you create a Theme on your own, isValid() will
 * return \c false, and all colors provided by this Theme are in fact invalid
 * and therefore unusable.
 *
 * \sa Format
 * \since 5.28
 */
class KSYNTAXHIGHLIGHTING_EXPORT Theme
{
    Q_GADGET

    /*!
     * \property KSyntaxHighlighting::Theme::name
     */
    Q_PROPERTY(QString name READ name)

    /*!
     * \property KSyntaxHighlighting::Theme::translatedName
     */
    Q_PROPERTY(QString translatedName READ translatedName)
public:
    /*!
     * Default styles that can be referenced from syntax definition XML files.
     * Make sure to choose readable colors with good contrast especially in
     * combination with the EditorColorRole%s.
     *
     * \value Normal
     *        Default text style for normal text and source code without
     *        special highlighting.
     * \value Keyword
     *        Text style for language keywords.
     * \value Function
     *        Text style for function definitions and function calls.
     * \value Variable
     *        Text style for variables, if applicable. For instance, variables in
     *        PHP typically start with a '$', so all identifiers following the
     *        pattern $foo are highlighted as variable.
     * \value ControlFlow
     *        Text style for control flow highlighting, such as if, then,
     *        else, return, or continue.
     * \value Operator
     *        Text style for operators such as +, -, *, / and :: etc.
     * \value BuiltIn
     *        Text style for built-in language classes and functions.
     * \value Extension
     *        Text style for well-known extensions, such as Qt or boost.
     * \value Preprocessor
     *        Text style for preprocessor statements.
     * \value Attribute
     *        Text style for attributes of functions or objects, e.g. \@override
     *        in Java, or __declspec(...) and __attribute__((...)) in C++.
     * \value Char
     *        Text style for single characters such as 'a'.
     * \value SpecialChar
     *        Text style for escaped characters in strings, such as "hello\n".
     * \value String
     *        Text style for strings, for instance "hello world".
     * \value VerbatimString
     *        Text style for verbatim strings such as HERE docs.
     * \value SpecialString
     *        Text style for special strings such as regular expressions in
     *        ECMAScript or the LaTeX math mode.
     * \value Import
     *        Text style for includes, imports, modules, or LaTeX packages.
     * \value DataType
     *        Text style for data types such as int, char, float etc.
     * \value DecVal
     *        Text style for decimal values.
     * \value BaseN
     *        Text style for numbers with base other than 10.
     * \value Float
     *        Text style for floating point numbers.
     * \value Constant
     *        Text style for language constants, e.g. True, False, None in Python
     *        or nullptr in C/C++.
     * \value Comment
     *        Text style for normal comments.
     * \value Documentation
     *        Text style for comments that reflect API documentation, such as
     *        doxygen comments.
     * \value Annotation
     *        Text style for annotations in comments, such as \a in Doxygen
     *        or JavaDoc.
     * \value CommentVar
     *        Text style that refers to variables in a comment, such as after
     *        \a \<identifier\> in Doxygen or JavaDoc.
     * \value RegionMarker
     *        Text style for region markers, typically defined by BEGIN/END.
     * \value Information
     *        Text style for information, such as the keyword \\note in Doxygen.
     * \value Warning
     *        Text style for warnings, such as the keyword \\warning in Doxygen.
     * \value Alert
     *        Text style for comment specials such as TODO and WARNING in
     *        comments.
     * \value Error
     *        Text style indicating wrong syntax.
     * \value Others
     *        Text style for attributes that do not match any of the other default
     *        styles.
     */
    enum TextStyle {
        Normal = 0,
        Keyword,
        Function,
        Variable,
        ControlFlow,
        Operator,
        BuiltIn,
        Extension,
        Preprocessor,
        Attribute,
        Char,
        SpecialChar,
        String,
        VerbatimString,
        SpecialString,
        Import,
        DataType,
        DecVal,
        BaseN,
        Float,
        Constant,
        Comment,
        Documentation,
        Annotation,
        CommentVar,
        RegionMarker,
        Information,
        Warning,
        Alert,
        Error,
        Others
    };
    Q_ENUM(TextStyle)

    /*!
     * Editor color roles, used to paint line numbers, editor background etc.
     * The colors typically should have good contrast with the colors used
     * in the TextStyle%s.
     *
     * \value BackgroundColor
     *        Background color for the editing area.
     * \value TextSelection
     *        Background color for selected text.
     * \value CurrentLine
     *        Background color for the line of the current text cursor.
     * \value SearchHighlight
     *        Background color for matching text while searching.
     * \value ReplaceHighlight
     *        Background color for replaced text for a search & replace action.
     * \value BracketMatching
     *        Background color for matching bracket pairs (including quotes).
     * \value TabMarker
     *        Foreground color for visualizing tabs and trailing spaces.
     * \value SpellChecking
     *        Color used to underline spell check errors.
     * \value IndentationLine
     *        Color used to draw vertical indentation levels, typically a line.
     * \value IconBorder
     *        Background color for the icon border.
     * \value CodeFolding
     *        Background colors for code folding regions in the text area, as well
     *        as code folding indicators in the code folding border.
     * \value LineNumbers
     *        Foreground color for drawing the line numbers. This should have a
     *        good contrast with the IconBorder background color.
     * \value CurrentLineNumber
     *        Foreground color for drawing the current line number. This should
     *        have a good contrast with the IconBorder background color.
     * \value WordWrapMarker
     *        Color used in the icon border to indicate dynamically wrapped lines.
     *        This color should have a good contrast with the IconBorder
     *        background color.
     * \value ModifiedLines
     *        Color used to draw a vertical line for marking changed lines.
     * \value SavedLines
     *        Color used to draw a vertical line for marking saved lines.
     * \value Separator
     *        Line color used to draw separator lines, e.g. at column 80 in the
     *        text editor area.
     * \value MarkBookmark
     *        Background color for bookmarks.
     * \value MarkBreakpointActive
     *        Background color for active breakpoints.
     * \value MarkBreakpointReached
     *        Background color for a reached breakpoint.
     * \value MarkBreakpointDisabled
     *        Background color for inactive (disabled) breakpoints.
     * \value MarkExecution
     *        Background color for marking the current execution position.
     * \value MarkWarning
     *        Background color for general warning marks.
     * \value MarkError
     *        Background color for general error marks.
     * \value TemplateBackground
     *        Background color for text templates (snippets).
     * \value TemplatePlaceholder
     *        Background color for all editable placeholders in text templates.
     * \value TemplateFocusedPlaceholder
     *        Background color for the currently active placeholder in text
     *        templates.
     * \value TemplateReadOnlyPlaceholder
     *        Background color for read-only placeholders in text templates.
     */
    enum EditorColorRole {
        BackgroundColor = 0,
        TextSelection,
        CurrentLine,
        SearchHighlight,
        ReplaceHighlight,
        BracketMatching,
        TabMarker,
        SpellChecking,
        IndentationLine,
        IconBorder,
        CodeFolding,
        LineNumbers,
        CurrentLineNumber,
        WordWrapMarker,
        ModifiedLines,
        SavedLines,
        Separator,
        MarkBookmark,
        MarkBreakpointActive,
        MarkBreakpointReached,
        MarkBreakpointDisabled,
        MarkExecution,
        MarkWarning,
        MarkError,
        TemplateBackground,
        TemplatePlaceholder,
        TemplateFocusedPlaceholder,
        TemplateReadOnlyPlaceholder
    };
    Q_ENUM(EditorColorRole)

    /*!
     * Default constructor, creating an invalid Theme, see isValid().
     */
    Theme();

    /*!
     * Copy constructor, sharing the Theme data with \a copy.
     */
    Theme(const Theme &copy);

    ~Theme();

    /*!
     * Assignment operator, sharing the Theme data with \a other.
     */
    Theme &operator=(const Theme &other);

    /*!
     * Returns \c true if this is a valid Theme.
     * If the theme is invalid, none of the returned colors are well-defined.
     */
    bool isValid() const;

    /*!
     * Returns the unique name of this Theme.
     * \sa translatedName()
     */
    QString name() const;

    /*!
     * Returns the translated name of this Theme. The translated name can be
     * used in the user interface.
     */
    QString translatedName() const;

    /*!
     * Returns \c true if this Theme is read-only.
     *
     * A Theme is read-only, if the filePath() points to a non-writable file.
     * This is typically the case for Themes that are compiled into the executable
     * as resource file, as well as for theme files that are installed in read-only
     * system locations (e.g. /usr/share/).
     */
    bool isReadOnly() const;

    /*!
     * Returns the full path and file name to this Theme.
     * Themes from the Qt resource return the Qt resource path.
     * Themes from disk return the local path.
     *
     * If the theme is invalid (isValid()), an empty string is returned.
     */
    QString filePath() const;

    /*!
     * Returns the text color to be used for \a style.
     * \c 0 is returned for styles that do not specify a text color,
     * use the default text color in that case.
     */
    QRgb textColor(TextStyle style) const;

    /*!
     * Returns the selected text color to be used for \a style.
     * \c 0 is returned for styles that do not specify a selected text color,
     * use the default textColor() in that case.
     */
    QRgb selectedTextColor(TextStyle style) const;

    /*!
     * Returns the background color to be used for \a style.
     * \c 0 is returned for styles that do not specify a background color,
     * use the default background color in that case.
     */
    QRgb backgroundColor(TextStyle style) const;

    /*!
     * Returns the background color to be used for selected text for \a style.
     * \c 0 is returned for styles that do not specify a background color,
     * use the default backgroundColor() in that case.
     */
    QRgb selectedBackgroundColor(TextStyle style) const;

    /*!
     * Returns whether the given style should be shown in bold.
     */
    bool isBold(TextStyle style) const;

    /*!
     * Returns whether the given style should be shown in italic.
     */
    bool isItalic(TextStyle style) const;

    /*!
     * Returns whether the given style should be shown underlined.
     */
    bool isUnderline(TextStyle style) const;

    /*!
     * Returns whether the given style should be shown struck through.
     */
    bool isStrikeThrough(TextStyle style) const;

public:
    /*!
     * Returns the editor color for the requested \a role.
     */
    QRgb editorColor(EditorColorRole role) const;

private:
    /*!
     * Constructor taking a shared ThemeData instance.
     */
    KSYNTAXHIGHLIGHTING_NO_EXPORT explicit Theme(ThemeData *data);
    friend class RepositoryPrivate;
    friend class ThemeData;

private:
    /*!
     * Shared data holder.
     */
    QExplicitlySharedDataPointer<ThemeData> m_data;
};

}

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(KSyntaxHighlighting::Theme, Q_RELOCATABLE_TYPE);
QT_END_NAMESPACE

#endif // KSYNTAXHIGHLIGHTING_THEME_H
