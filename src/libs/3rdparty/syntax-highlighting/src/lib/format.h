/*
    Copyright (C) 2016 Volker Krause <vkrause@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef KSYNTAXHIGHLIGHTING_FORMAT_H
#define KSYNTAXHIGHLIGHTING_FORMAT_H

#include "ksyntaxhighlighting_export.h"
#include "theme.h"

#include <QExplicitlySharedDataPointer>
#include <QTypeInfo>

QT_BEGIN_NAMESPACE
class QColor;
class QString;
class QXmlStreamReader;
QT_END_NAMESPACE

namespace KSyntaxHighlighting {

class DefinitionRef;
class FormatPrivate;

/** Describes the format to be used for a specific text fragment.
 *  The actual format used for displaying is merged from the format information
 *  in the syntax definition file, and a theme.
 *
 *  @see Theme
 *  @since 5.28
 */
class KSYNTAXHIGHLIGHTING_EXPORT Format
{
public:
    /** Creates an empty/invalid format. */
    Format();
    Format(const Format &other);
    ~Format();

    Format& operator=(const Format &other);

    /** Returns @c true if this is a valid format, ie. one that
     *  was read from a syntax definition file.
     */
    bool isValid() const;

    /** The name of this format as used in the syntax definition file. */
    QString name() const;

    /** Returns a unique identifier of this format.
     *  This is useful for efficient storing of formats in a text line. The
     *  identifier is unique per Repository instance, but will change when
     *  the repository is reloaded (which also invalidatess the corresponding
     *  Definition anyway).
     */
    quint16 id() const;

    /** Returns the underlying TextStyle of this Format.
     *  Every Theme::TextStyle is visually defined by a Theme. A Format uses one
     *  of the Theme::TextStyle%s and on top allows modifications such as setting
     *  a different foreground color etc.
     *  @see Theme::TextStyle
     *  @since 5.49
     */
    Theme::TextStyle textStyle() const;

    /** Returns @c true if the combination of this format and the theme @p theme
     *  do not change the default text format in any way.
     *  This is useful for output formats where changing formatting implies cost,
     *  and thus benefit from optimizing the default case of not having any format
     *  applied. If you make use of this, make sure to set the default text style
     *  to what the corresponding theme sets for Theme::Normal.
     */
    bool isDefaultTextStyle(const Theme &theme) const;

    /** Returns @c true if the combination of this format and the theme @p theme
     *  change the foreground color compared to the default format.
     */
    bool hasTextColor(const Theme &theme) const;
    /** Returns the foreground color of the combination of this format and the
     *  given theme.
     */
    QColor textColor(const Theme &theme) const;
    /** Returns the foreground color for selected text of the combination of
     *  this format and the given theme.
     */
    QColor selectedTextColor(const Theme &theme) const;
    /** Returns @c true if the combination of this format and the theme @p theme
     *  change the background color compared to the default format.
     */
    bool hasBackgroundColor(const Theme &theme) const;
    /** Returns the background color of the combination of this format and the
     *  given theme.
     */
    QColor backgroundColor(const Theme &theme) const;
    /** Returns the background color of selected text of the combination of
     *  this format and the given theme.
     */
    QColor selectedBackgroundColor(const Theme &theme) const;

    /** Returns @c true if the combination of this format and the given theme
     *  results in bold text formatting.
     */
    bool isBold(const Theme &theme) const;
    /** Returns @c true if the combination of this format and the given theme
     *  results in italic text formatting.
     */
    bool isItalic(const Theme &theme) const;
    /** Returns @c true if the combination of this format and the given theme
     *  results in underlined text.
     */
    bool isUnderline(const Theme &theme) const;
    /** Returns @c true if the combination of this format and the given theme
     *  results in struck through text.
     */
    bool isStrikeThrough(const Theme &theme) const;

    /**
     * Returns whether characters with this format should be spell checked.
     */
    bool spellCheck() const;

private:
    friend class FormatPrivate;
    QExplicitlySharedDataPointer<FormatPrivate> d;
};
}

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(KSyntaxHighlighting::Format, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

#endif // KSYNTAXHIGHLIGHTING_FORMAT_H
