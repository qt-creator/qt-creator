/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_ABSTRACTHIGHLIGHTERM_H
#define KSYNTAXHIGHLIGHTING_ABSTRACTHIGHLIGHTERM_H

#include "definition.h"
#include "ksyntaxhighlighting_export.h"

#include <QObject>
#include <QStringView>

namespace KSyntaxHighlighting
{
class AbstractHighlighterPrivate;
class FoldingRegion;
class Format;
class State;
class Theme;

/*!
 * \class KSyntaxHighlighting::AbstractHighlighter
 * \inheaderfile KSyntaxHighlighting/AbstractHighlighter
 * \inmodule KSyntaxHighlighting
 *
 * \brief Abstract base class for highlighters.
 *
 * The AbstractHighlighter provides an interface to highlight text.
 *
 * The SyntaxHighlighting framework already ships with one implementation,
 * namely the SyntaxHighlighter, which also derives from QSyntaxHighlighter,
 * meaning that it can be used to highlight a QTextDocument or a QML TextEdit.
 * In order to use the SyntaxHighlighter, just call setDefinition() and
 * setTheme(), and the associated documents will automatically be highlighted.
 *
 * However, if you want to use the SyntaxHighlighting framework to implement
 * your own syntax highlighter, you need to sublcass from AbstractHighlighter.
 *
 * In order to implement your own syntax highlighter, you need to inherit from
 * AbstractHighlighter. Then, pass each text line that needs to be highlighted
 * in order to highlightLine(). Internally, highlightLine() uses the Definition
 * initially set through setDefinition() and the State of the previous text line
 * to parse and highlight the given text line. For each visual highlighting
 * change, highlightLine() will call applyFormat(). Therefore, reimplement
 * applyFormat() to get notified of the Format that is valid in the range
 * starting at the given offset with the specified length. Similarly, for each
 * text part that starts or ends a code folding region, highlightLine() will
 * call applyFolding(). Therefore, if you are interested in code folding,
 * reimplement applyFolding() to get notified of the starting and ending code
 * folding regions, again specified in the range starting at the given offset
 * with the given length.
 *
 * The Format class itself depends on the current Theme. A theme must be
 * initially set once such that the Format%s instances can be queried for
 * concrete colors.
 *
 * Optionally, you can also reimplement setTheme() and setDefinition() to get
 * notified whenever the Definition or the Theme changes.
 *
 * \sa SyntaxHighlighter
 * \since 5.28
 */
class KSYNTAXHIGHLIGHTING_EXPORT AbstractHighlighter
{
public:
    virtual ~AbstractHighlighter();

    /*!
     * Returns the syntax definition used for highlighting.
     *
     * \sa setDefinition()
     */
    Definition definition() const;

    /*!
     * Sets the syntax definition used for highlighting.
     *
     * Subclasses can re-implement this method to e.g. trigger
     * re-highlighting or clear internal data structures if needed.
     */
    virtual void setDefinition(const Definition &def);

    /*!
     * Returns the currently selected theme for highlighting.
     *
     * \note If no Theme was set through setTheme(), the returned Theme will be
     *       invalid, see Theme::isValid().
     */
    Theme theme() const;

    /*!
     * Sets the theme used for highlighting.
     *
     * Subclasses can re-implement this method to e.g. trigger
     * re-highlighing or to do general palette color setup.
     */
    virtual void setTheme(const Theme &theme);

protected:
    /*!
     */
    AbstractHighlighter();

    KSYNTAXHIGHLIGHTING_NO_EXPORT explicit AbstractHighlighter(AbstractHighlighterPrivate *dd);

    /*!
     * Highlight the given line. Call this from your derived class
     * where appropriate. This will result in any number of applyFormat()
     * and applyFolding() calls as a result.
     *
     * \a text A string containing the text of the line to highlight.
     *
     * \a state The highlighting state handle returned by the call
     *        to highlightLine() for the previous line. For the very first line,
     *        just pass a default constructed State().
     *
     * Returns The state of the highlighting engine after processing the
     *        given line. This needs to passed into highlightLine() for the
     *        next line. You can store the state for efficient partial
     *        re-highlighting for example during editing.
     *
     * \sa applyFormat(), applyFolding()
     */
    State highlightLine(QStringView text, const State &state);

    /*!
     * Reimplement this to apply formats to your output. The provided \a format
     * is valid for the interval [ \a offset, \a offset + \a length).
     *
     * \a offset The start column of the interval for which \a format matches
     *
     * \a length The length of the matching text
     *
     * \a format The Format that applies to the range [offset, offset + length)
     *
     * \note Make sure to set a valid Definition, otherwise the parameter
     *       \a format is invalid for the entire line passed to highlightLine()
     *       (cf. Format::isValid()).
     *
     * \sa applyFolding(), highlightLine()
     */
    virtual void applyFormat(int offset, int length, const Format &format) = 0;

    /*!
     * Reimplement this to apply folding to your output. The provided
     * FoldingRegion \a region either stars or ends a code folding region in the
     * interval [ \a offset, \a offset + \a length).
     *
     * \a offset The start column of the FoldingRegion
     *
     * \a length The length of the matching text that starts / ends a
     *       folding region
     *
     * \a region The FoldingRegion that applies to the range [offset, offset + length)
     *
     * \note The FoldingRegion \a region is @e always either of type
     *       FoldingRegion::Type::Begin or FoldingRegion::Type::End.
     *
     * \sa applyFormat(), highlightLine(), FoldingRegion
     */
    virtual void applyFolding(int offset, int length, FoldingRegion region);

protected:
    AbstractHighlighterPrivate *d_ptr;

private:
    Q_DECLARE_PRIVATE(AbstractHighlighter)
    Q_DISABLE_COPY(AbstractHighlighter)
};
}

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(KSyntaxHighlighting::AbstractHighlighter, "org.kde.SyntaxHighlighting.AbstractHighlighter")
QT_END_NAMESPACE

#endif // KSYNTAXHIGHLIGHTING_ABSTRACTHIGHLIGHTERM_H
