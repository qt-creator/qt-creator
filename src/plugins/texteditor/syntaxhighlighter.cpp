// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "syntaxhighlighter.h"
#include "textdocumentlayout.h"
#include "texteditorsettings.h"
#include "fontsettings.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QPointer>
#include <QTextDocument>
#include <QThread>

#include <cmath>

namespace TextEditor {

enum HighlighterTypeProperty
{
    SyntaxHighlight = QTextFormat::UserProperty + 1,
    SemanticHighlight = QTextFormat::UserProperty + 2
};

class SyntaxHighlighterPrivate
{
public:
    SyntaxHighlighterPrivate() = default;

    SyntaxHighlighterPrivate(const FontSettings &fontSettings)
    {
        updateFormats(fontSettings);
    }

    QPointer<QTextDocument> doc;

    void updateFormats(int from, int charsRemoved, int charsAdded);
    void reformatBlocks(int from, int charsRemoved, int charsAdded);
    void reformatBlocks();
    void reformatBlock(const QTextBlock &block);

    inline void rehighlight(QTextCursor &cursor, QTextCursor::MoveOperation operation) {
        inReformatBlocks = true;
        int from = cursor.position();
        cursor.movePosition(operation);
        reformatBlocks(from, 0, cursor.position() - from);
        inReformatBlocks = false;
    }

    void applyFormatChanges();
    void updateFormats(const FontSettings &fontSettings);

    FontSettings fontSettings;
    QList<QTextCharFormat> formatChanges;
    QTextBlock currentBlock;
    bool rehighlightPending = false;
    bool inReformatBlocks = false;
    TextDocumentLayout::FoldValidator foldValidator;
    QList<QTextCharFormat> formats;
    QList<std::pair<int,TextStyle>> formatCategories;
    QTextCharFormat whitespaceFormat;
    QString mimeType;
    bool syntaxInfoUpToDate = false;
    int highlightStartBlock = 0;
    int highlightEndBlock = 0;
    QSet<int> forceRehighlightBlocks;
    SyntaxHighlighter *q;
};

static bool adjustRange(QTextLayout::FormatRange &range, int from, int charsDelta)
{
    if (range.start >= from) {
        range.start += charsDelta;
        return true;
    } else if (range.start + range.length > from) {
        range.length += charsDelta;
        return true;
    }
    return false;
}

void SyntaxHighlighter::delayedRehighlight()
{
    if (!d->rehighlightPending)
        return;
    d->rehighlightPending = false;

    rehighlight();
}

void SyntaxHighlighter::continueRehighlight()
{
    d->reformatBlocks();
}

#ifdef WITH_TESTS
SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent, const FontSettings &fontsettings)
    : QObject(parent), d(new SyntaxHighlighterPrivate(fontsettings))
{
    d->q = this;
    if (parent)
        setDocument(parent);
}
#endif

void SyntaxHighlighterPrivate::applyFormatChanges()
{
    QTextLayout *layout = currentBlock.layout();

    QList<QTextLayout::FormatRange> ranges;
    QList<QTextLayout::FormatRange> oldRanges;
    std::tie(oldRanges, ranges)
        = Utils::partition(layout->formats(), [](const QTextLayout::FormatRange &range) {
              return range.format.property(SyntaxHighlight).toBool();
          });

    QTextCharFormat emptyFormat;

    QTextLayout::FormatRange r;

    QList<QTextLayout::FormatRange> newRanges;
    int i = 0;
    while (i < formatChanges.count()) {

        while (i < formatChanges.count() && formatChanges.at(i) == emptyFormat)
            ++i;

        if (i >= formatChanges.count())
            break;

        r.start = i;
        r.format = formatChanges.at(i);

        while (i < formatChanges.count() && formatChanges.at(i) == r.format)
            ++i;

        r.format.setProperty(SyntaxHighlight, true);
        r.length = i - r.start;

        const QString preeditText = currentBlock.layout()->preeditAreaText();
        if (!preeditText.isEmpty()) {
            const int preeditPosition = currentBlock.layout()->preeditAreaPosition();
            if (r.start >= preeditPosition) {
                r.start += preeditText.length();
            } else if (r.start + r.length > preeditPosition) {
                QTextLayout::FormatRange beforePreeditRange = r;
                r.start = preeditPosition + preeditText.length();
                r.length = r.length - (r.start - preeditPosition);
                beforePreeditRange.length = preeditPosition - beforePreeditRange.start;
                newRanges << beforePreeditRange;
            }
        }

        newRanges << r;
    }

    bool formatsChanged = (newRanges.size() != oldRanges.size());

    for (int i = 0; !formatsChanged && i < newRanges.size(); ++i) {
        const QTextLayout::FormatRange &o = oldRanges.at(i);
        const QTextLayout::FormatRange &n = newRanges.at(i);
        formatsChanged = (o.start != n.start || o.length != n.length || o.format != n.format);
    }

    if (formatsChanged) {
        ranges.append(newRanges);
        layout->setFormats(ranges);
        doc->markContentsDirty(currentBlock.position(), currentBlock.length());
    }
}

void SyntaxHighlighter::reformatBlocks(int from, int charsRemoved, int charsAdded)
{
    if (!d->inReformatBlocks)
        d->reformatBlocks(from, charsRemoved, charsAdded);
}

void SyntaxHighlighterPrivate::updateFormats(int from, int charsRemoved, int charsAdded)
{
    bool formatsChanged = false;

    const QTextBlock block = doc->findBlock(from);
    QTextLayout *layout = block.layout();

    QList<QTextLayout::FormatRange> ranges = layout->formats();

    const int charsDelta = charsAdded - charsRemoved;
    for (QTextLayout::FormatRange &range : ranges)
        formatsChanged |= adjustRange(range, from - block.position(), charsDelta);

    if (formatsChanged) {
        layout->setFormats(ranges);
        doc->markContentsDirty(block.position(), block.length());
    }
}

void SyntaxHighlighterPrivate::reformatBlocks(int from, int charsRemoved, int charsAdded)
{
    updateFormats(from, charsRemoved, charsAdded);

    QTextBlock block = doc->findBlock(from);
    if (block.isValid() && block.blockNumber() < highlightStartBlock)
        highlightStartBlock = block.blockNumber();
    block = doc->findBlock(from + charsAdded + (charsRemoved > 0 ? 1 : 0));
    if (!block.isValid())
        highlightEndBlock = doc->blockCount() - 1;
    else if (block.blockNumber() > highlightEndBlock)
        highlightEndBlock = block.blockNumber();

    reformatBlocks();
}

void SyntaxHighlighterPrivate::reformatBlocks()
{
    QElapsedTimer et;
    et.start();

    syntaxInfoUpToDate = false;
    rehighlightPending = false;

    foldValidator.reset();

    bool forceHighlightOfNextBlock = false;
    QTextBlock block = doc->findBlockByNumber(highlightStartBlock);
    QTC_ASSERT(block.isValid(), block = doc->firstBlock());
    QTextBlock endBlock = doc->findBlockByNumber(highlightEndBlock);
    QTC_ASSERT(endBlock.isValid(), endBlock = doc->lastBlock());

    while (block.isValid()) {
        if (et.elapsed() > 20)
            break;

        const int stateBeforeHighlight = block.userState();

        if (forceHighlightOfNextBlock || forceRehighlightBlocks.contains(block.blockNumber())
                || block.blockNumber() <= highlightEndBlock) {
            reformatBlock(block);
            forceRehighlightBlocks.remove(block.blockNumber());
            forceHighlightOfNextBlock = (block.userState() != stateBeforeHighlight);
        }
        highlightStartBlock = block.blockNumber();

        if (block == endBlock && !forceHighlightOfNextBlock)
            break;
        block = block.next();
    }

    formatChanges.clear();
    foldValidator.finalize();

    if (endBlock.isValid() && block.isValid() && block.blockNumber() < endBlock.blockNumber()) {
        QMetaObject::invokeMethod(q, &SyntaxHighlighter::continueRehighlight, Qt::QueuedConnection);
        if (forceHighlightOfNextBlock)
            forceRehighlightBlocks << block.blockNumber();
    } else {
        highlightEndBlock = 0;
        syntaxInfoUpToDate = true;
        emit q->finished();
    }
}

void SyntaxHighlighterPrivate::reformatBlock(const QTextBlock &block)
{
    QTC_ASSERT(!currentBlock.isValid(), return);

    currentBlock = block;

    formatChanges.fill(QTextCharFormat(), block.length() - 1);
    q->highlightBlock(block.text());
    applyFormatChanges();

    foldValidator.process(currentBlock);

    currentBlock = QTextBlock();
}

/*!
    \class SyntaxHighlighter

    \brief The SyntaxHighlighter class allows you to define syntax highlighting rules and to query
    a document's current formatting or user data.

    The SyntaxHighlighter class is a copied and forked version of the QSyntaxHighlighter. There are
    a couple of binary incompatible changes that prevent doing this directly in Qt.

    The main difference from the QSyntaxHighlighter is the addition of setExtraFormats.
    This method prevents redoing syntax highlighting when setting the formats on the
    layout and subsequently marking the document contents dirty. It thus prevents the redoing of the
    semantic highlighting, which sets extra formats, and so on.

    Another way to implement the semantic highlighting is to use ExtraSelections on
    Q(Plain)TextEdit. The drawback of QTextEdit::setExtraSelections is that ExtraSelection uses a
    QTextCursor for positioning. That means that with every document change (that is, every
    keystroke), a whole bunch of cursors can be re-checked or re-calculated. This is not needed in
    our situation, because the next thing that will happen is that the highlighting will come up
    with new ranges, meaning that it destroys the cursors. To make things worse, QTextCursor
    calculates the pixel position in the line it's in. The calculations are done with
    QTextLine::cursorTo, which is very expensive and is not optimized for fixed-width fonts. Another
    reason not to use ExtraSelections is that those selections belong to the editor, not to the
    document. This means that every editor needs a highlighter, instead of every document. This
    could become expensive when multiple editors with the same document are opened.

    So, we use formats, because all those highlights should get removed or redone soon
    after the change happens.
*/

/*!
    Constructs a SyntaxHighlighter with the given \a parent.
*/
SyntaxHighlighter::SyntaxHighlighter(QObject *parent)
    : QObject(parent), d(new SyntaxHighlighterPrivate)
{
    d->q = this;
}

/*!
    Constructs a SyntaxHighlighter and installs it on \a parent.
    The specified QTextDocument also becomes the owner of the
    SyntaxHighlighter.
*/
SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QObject(parent), d(new SyntaxHighlighterPrivate)
{
    d->q = this;
    if (parent)
        setDocument(parent);
}

/*!
    Constructs a SyntaxHighlighter and installs it on \a parent's
    QTextDocument. The specified QTextEdit also becomes the owner of
    the SyntaxHighlighter.
*/
SyntaxHighlighter::SyntaxHighlighter(QTextEdit *parent)
    : QObject(parent), d(new SyntaxHighlighterPrivate)
{
    d->q = this;
    if (parent)
        setDocument(parent->document());
}

/*!
    Destructor that uninstalls this syntax highlighter from the text document.
*/
SyntaxHighlighter::~SyntaxHighlighter()
{
    setDocument(nullptr);
}

/*!
    Installs the syntax highlighter on the given QTextDocument \a doc.
    A SyntaxHighlighter can only be used with one document at a time.
*/
void SyntaxHighlighter::setDocument(QTextDocument *doc)
{
    if (d->doc == doc)
        return;

    if (d->doc) {
        disconnect(d->doc, &QTextDocument::contentsChange, this, &SyntaxHighlighter::reformatBlocks);

        QTextCursor cursor(d->doc);
        cursor.beginEditBlock();
        for (QTextBlock blk = d->doc->begin(); blk.isValid(); blk = blk.next())
            blk.layout()->clearFormats();
        cursor.endEditBlock();
    }
    QTextDocument *oldDoc = d->doc;
    d->doc = doc;
    documentChanged(oldDoc, d->doc);
    if (d->doc) {
        connect(d->doc, &QTextDocument::contentsChange, this, &SyntaxHighlighter::reformatBlocks);
        scheduleRehighlight();
        d->foldValidator.setup(qobject_cast<TextDocumentLayout *>(doc->documentLayout()));
    }
}

/*!
    Returns the QTextDocument on which this syntax highlighter is
    installed.
*/
QTextDocument *SyntaxHighlighter::document() const
{
    return d->doc;
}

void SyntaxHighlighter::setMimeType(const QString &mimeType)
{
    d->mimeType = mimeType;
}

QString SyntaxHighlighter::mimeType() const
{
    return d->mimeType;
}

/*!
    \since 4.2

    Reapplies the highlighting to the whole document.

    \sa rehighlightBlock()
*/
void SyntaxHighlighter::rehighlight()
{
    if (!d->doc)
        return;

    QTextCursor cursor(d->doc);
    d->rehighlight(cursor, QTextCursor::End);
}

void SyntaxHighlighter::scheduleRehighlight()
{
    if (d->rehighlightPending)
        return;
    d->rehighlightPending = true;
    d->syntaxInfoUpToDate = false;
    QMetaObject::invokeMethod(this,
                              &SyntaxHighlighter::delayedRehighlight,
                              Qt::QueuedConnection);
}

/*!
    \since 4.6

    Reapplies the highlighting to the given QTextBlock \a block.

    \sa rehighlight()
*/
void SyntaxHighlighter::rehighlightBlock(const QTextBlock &block)
{
    if (!d->doc || !block.isValid() || block.document() != d->doc)
        return;

    const bool rehighlightPending = d->rehighlightPending;

    QTextCursor cursor(block);
    d->rehighlight(cursor, QTextCursor::EndOfBlock);

    if (rehighlightPending)
        d->rehighlightPending = rehighlightPending;
}

/*!
    \fn void SyntaxHighlighter::highlightBlock(const QString &text)

    Highlights the given text block. This function is called when
    necessary by the rich text engine, i.e. on text blocks which have
    changed.

    To provide your own syntax highlighting, you must subclass
    SyntaxHighlighter and reimplement highlightBlock(). In your
    reimplementation you should parse the block's \a text and call
    setFormat() as often as necessary to apply any font and color
    changes that you require. For example:

    \snippet doc/src/snippets/code/src_gui_text_SyntaxHighlighter.cpp 3

    Some syntaxes can have constructs that span several text
    blocks. For example, a C++ syntax highlighter should be able to
    cope with \c{/}\c{*...*}\c{/} multiline comments. To deal with
    these cases it is necessary to know the end state of the previous
    text block (e.g. "in comment").

    Inside your highlightBlock() implementation you can query the end
    state of the previous text block using the previousBlockState()
    function. After parsing the block you can save the last state
    using setCurrentBlockState().

    The currentBlockState() and previousBlockState() functions return
    an int value. If no state is set, the returned value is -1. You
    can designate any other value to identify any given state using
    the setCurrentBlockState() function. Once the state is set the
    QTextBlock keeps that value until it is set set again or until the
    corresponding paragraph of text gets deleted.

    For example, if you're writing a simple C++ syntax highlighter,
    you might designate 1 to signify "in comment". For a text block
    that ended in the middle of a comment you'd set 1 using
    setCurrentBlockState, and for other paragraphs you'd set 0.
    In your parsing code if the return value of previousBlockState()
    is 1, you would highlight the text as a C++ comment until you
    reached the closing \c{*}\c{/}.

    \sa previousBlockState(), setFormat(), setCurrentBlockState()
*/

/*!
    This function is applied to the syntax highlighter's current text
    block (i.e. the text that is passed to the highlightBlock()
    function).

    The specified \a format is applied to the text from the \a start
    position for a length of \a count characters (if \a count is 0,
    nothing is done). The formatting properties set in \a format are
    merged at display time with the formatting information stored
    directly in the document, for example as previously set with
    QTextCursor's functions. Note that the document itself remains
    unmodified by the format set through this function.

    \sa format(), highlightBlock()
*/
void SyntaxHighlighter::setFormat(int start, int count, const QTextCharFormat &format)
{
    if (start < 0 || start >= d->formatChanges.count())
        return;

    const int end = qMin(start + count, d->formatChanges.count());
    for (int i = start; i < end; ++i)
        d->formatChanges[i] = format;
}

/*!
    \overload

    The specified \a color is applied to the current text block from
    the \a start position for a length of \a count characters.

    The other attributes of the current text block, e.g. the font and
    background color, are reset to default values.

    \sa format(), highlightBlock()
*/
void SyntaxHighlighter::setFormat(int start, int count, const QColor &color)
{
    QTextCharFormat format;
    format.setForeground(color);
    setFormat(start, count, format);
}

/*!
    \overload

    The specified \a font is applied to the current text block from
    the \a start position for a length of \a count characters.

    The other attributes of the current text block, e.g. the font and
    background color, are reset to default values.

    \sa format(), highlightBlock()
*/
void SyntaxHighlighter::setFormat(int start, int count, const QFont &font)
{
    QTextCharFormat format;
    format.setFont(font);
    setFormat(start, count, format);
}

void SyntaxHighlighter::formatSpaces(const QString &text, int start, int count)
{
    int offset = start;
    const int end = std::min(start + count, int(text.length()));
    while (offset < end) {
        if (text.at(offset).isSpace()) {
            int start = offset++;
            while (offset < end && text.at(offset).isSpace())
                ++offset;
            setFormat(start, offset - start, d->whitespaceFormat);
        } else {
            ++offset;
        }
    }
}

/*!
    The specified \a format is applied to all non-whitespace characters in the current text block
    with \a text, from the \a start position for a length of \a count characters.
    Whitespace characters are formatted with the visual whitespace format, merged with the
    non-whitespace format.

    \sa setFormat()
*/
void SyntaxHighlighter::setFormatWithSpaces(const QString &text, int start, int count,
                                            const QTextCharFormat &format)
{
    const QTextCharFormat visualSpaceFormat = whitespacified(format);

    const int end = std::min(start + count, int(text.length()));
    int index = start;

    while (index != end) {
        const bool isSpace = text.at(index).isSpace();
        const int start = index;

        do { ++index; }
        while (index != end && text.at(index).isSpace() == isSpace);

        const int tokenLength = index - start;
        if (isSpace)
            setFormat(start, tokenLength, visualSpaceFormat);
        else if (format.isValid())
            setFormat(start, tokenLength, format);
    }
}

/*!
    Returns the format at \a position inside the syntax highlighter's
    current text block.
*/
QTextCharFormat SyntaxHighlighter::format(int pos) const
{
    return d->formatChanges.value(pos);
}

/*!
    Returns the end state of the text block previous to the
    syntax highlighter's current block. If no value was
    previously set, the returned value is -1.

    \sa highlightBlock(), setCurrentBlockState()
*/
int SyntaxHighlighter::previousBlockState() const
{
    if (!d->currentBlock.isValid())
        return -1;

    const QTextBlock previous = d->currentBlock.previous();
    if (!previous.isValid())
        return -1;

    return previous.userState();
}

/*!
    Returns the state of the current text block. If no value is set,
    the returned value is -1.
*/
int SyntaxHighlighter::currentBlockState() const
{
    if (!d->currentBlock.isValid())
        return -1;

    return d->currentBlock.userState();
}

/*!
    Sets the state of the current text block to \a newState.

    \sa highlightBlock()
*/
void SyntaxHighlighter::setCurrentBlockState(int newState)
{
    if (!d->currentBlock.isValid())
        return;

    d->currentBlock.setUserState(newState);
}

/*!
    Attaches the given \a data to the current text block.  The
    ownership is passed to the underlying text document, i.e. the
    provided QTextBlockUserData object will be deleted if the
    corresponding text block gets deleted.

    QTextBlockUserData can be used to store custom settings. In the
    case of syntax highlighting, it is in particular interesting as
    cache storage for information that you may figure out while
    parsing the paragraph's text.

    For example while parsing the text, you can keep track of
    parenthesis characters that you encounter ('{[(' and the like),
    and store their relative position and the actual QChar in a simple
    class derived from QTextBlockUserData:

    \snippet doc/src/snippets/code/src_gui_text_SyntaxHighlighter.cpp 4

    During cursor navigation in the associated editor, you can ask the
    current QTextBlock (retrieved using the QTextCursor::block()
    function) if it has a user data object set and cast it to your \c
    BlockData object. Then you can check if the current cursor
    position matches with a previously recorded parenthesis position,
    and, depending on the type of parenthesis (opening or closing),
    find the next opening or closing parenthesis on the same level.

    In this way you can do a visual parenthesis matching and highlight
    from the current cursor position to the matching parenthesis. That
    makes it easier to spot a missing parenthesis in your code and to
    find where a corresponding opening/closing parenthesis is when
    editing parenthesis intensive code.

    \sa QTextBlock::setUserData()
*/
void SyntaxHighlighter::setCurrentBlockUserData(QTextBlockUserData *data)
{
    if (!d->currentBlock.isValid())
        return;

    d->currentBlock.setUserData(data);
}

/*!
    Returns the QTextBlockUserData object previously attached to the
    current text block.

    \sa QTextBlock::userData(), setCurrentBlockUserData()
*/
QTextBlockUserData *SyntaxHighlighter::currentBlockUserData() const
{
    if (!d->currentBlock.isValid())
        return nullptr;

    return d->currentBlock.userData();
}

/*!
    \since 4.4

    Returns the current text block.
*/
QTextBlock SyntaxHighlighter::currentBlock() const
{
    return d->currentBlock;
}

static bool byStartOfRange(const QTextLayout::FormatRange &range, const QTextLayout::FormatRange &other)
{
    return range.start < other.start;
}

void SyntaxHighlighter::setExtraFormats(const QTextBlock &block,
                                        const QList<QTextLayout::FormatRange> &formats)
{
    QList<QTextLayout::FormatRange> formatsCopy = formats;

    const int blockLength = block.length();
    if (block.layout() == nullptr || blockLength == 0)
        return;

    const QString preeditText = block.layout()->preeditAreaText();
    if (!preeditText.isEmpty()) {
        QList<QTextLayout::FormatRange> additionalRanges;
        const int preeditPosition = block.layout()->preeditAreaPosition();
        for (QTextLayout::FormatRange &r : formatsCopy) {
            if (r.start >= preeditPosition) {
                r.start += preeditText.length();
            } else if (r.start + r.length > preeditPosition) {
                QTextLayout::FormatRange afterPreeditRange = r;
                afterPreeditRange.start = preeditPosition + preeditText.length();
                afterPreeditRange.length = r.length - (preeditPosition - r.start);
                additionalRanges << afterPreeditRange;
                r.length = preeditPosition - r.start;
            }
        }
        formatsCopy << additionalRanges;
    }

    Utils::sort(formatsCopy, byStartOfRange);

    const QList<QTextLayout::FormatRange> all = block.layout()->formats();
    QList<QTextLayout::FormatRange> previousSemanticFormats;
    QList<QTextLayout::FormatRange> formatsToApply;
    std::tie(previousSemanticFormats, formatsToApply)
        = Utils::partition(all, [](const QTextLayout::FormatRange &r) {
              return r.format.property(SemanticHighlight).toBool();
          });

    for (auto &format : formatsCopy)
        format.format.setProperty(SemanticHighlight, true);

    if (formatsCopy.size() == previousSemanticFormats.size()) {
        Utils::sort(previousSemanticFormats, byStartOfRange);
        if (formats == previousSemanticFormats)
            return;
    }

    formatsToApply += formatsCopy;

    bool wasInReformatBlocks = d->inReformatBlocks;
    d->inReformatBlocks = true;
    block.layout()->setFormats(formatsToApply);

    document()->markContentsDirty(block.position(), blockLength - 1);
    d->inReformatBlocks = wasInReformatBlocks;
}

bool SyntaxHighlighter::syntaxHighlighterUpToDate() const
{
    return d->syntaxInfoUpToDate;
}

void SyntaxHighlighter::clearExtraFormats(const QTextBlock &block)
{
    const int blockLength = block.length();
    if (block.layout() == nullptr || blockLength == 0)
        return;

    const QList<QTextLayout::FormatRange> formatsToApply
        = Utils::filtered(block.layout()->formats(), [](const QTextLayout::FormatRange &r) {
              return !r.format.property(SemanticHighlight).toBool();
          });

    bool wasInReformatBlocks = d->inReformatBlocks;
    d->inReformatBlocks = true;
    block.layout()->setFormats(formatsToApply);

    document()->markContentsDirty(block.position(), blockLength - 1);
    d->inReformatBlocks = wasInReformatBlocks;
}

void SyntaxHighlighter::clearAllExtraFormats()
{
    QTextBlock b = document()->firstBlock();
    while (b.isValid()) {
        clearExtraFormats(b);
        b = b.next();
    }
}

/* Generate at least n different colors for highlighting, excluding background
 * color. */

QList<QColor> SyntaxHighlighter::generateColors(int n, const QColor &background)
{
    QList<QColor> result;
    // Assign a color gradient. Generate a sufficient number of colors
    // by using ceil and looping from 0..step.
    const double oneThird = 1.0 / 3.0;
    const int step = qRound(std::ceil(std::pow(double(n), oneThird)));
    result.reserve(step * step * step);
    const int factor = 255 / step;
    const int half = factor / 2;
    const int bgRed = background.red();
    const int bgGreen = background.green();
    const int bgBlue = background.blue();
    for (int r = step; r >= 0; --r) {
        const int red = r * factor;
        if (bgRed - half > red || bgRed + half <= red) {
            for (int g = step; g >= 0; --g) {
                const int green = g * factor;
                if (bgGreen - half > green || bgGreen + half <= green) {
                    for (int b = step; b >= 0 ; --b) {
                        const int blue = b * factor;
                        if (bgBlue - half > blue || bgBlue + half <= blue)
                            result.append(QColor(red, green, blue));
                    }
                }
            }
        }
    }
    return result;
}

void SyntaxHighlighter::setFontSettings(const FontSettings &fontSettings)
{
    d->updateFormats(fontSettings);
}

FontSettings SyntaxHighlighter::fontSettings() const
{
    return d->fontSettings;
}

/*!
    Creates text format categories for the text styles themselves, so the highlighter can
    use \c{formatForCategory(C_COMMENT)} and similar, and avoid creating its own format enum.
    \sa setTextFormatCategories()
*/
void SyntaxHighlighter::setDefaultTextFormatCategories()
{
    // map all text styles to themselves
    setTextFormatCategories(C_LAST_STYLE_SENTINEL, [](int i) { return TextStyle(i); });
}

/*!
    Uses the \a formatMapping function to create a mapping from the custom formats (the ints)
    to text styles. The \a formatMapping must handle all values from 0 to \a count.

    \sa setDefaultTextFormatCategories()
    \sa setTextFormatCategories()
*/
void SyntaxHighlighter::setTextFormatCategories(int count,
                                                std::function<TextStyle(int)> formatMapping)
{
    QList<std::pair<int, TextStyle>> categories;
    categories.reserve(count);
    for (int i = 0; i < count; ++i)
        categories.append({i, formatMapping(i)});
    setTextFormatCategories(categories);
}

/*!
    Creates a mapping between custom format enum values (the int values in the pairs) to
    text styles. Afterwards \c{formatForCategory(MyCustomFormatEnumValue)} can be used to
    efficiently retrieve the text style for a value.

    Note that this creates a vector with a size of the maximum int value in \a categories.

    \sa setDefaultTextFormatCategories()
*/
void SyntaxHighlighter::setTextFormatCategories(const QList<std::pair<int, TextStyle>> &categories)
{
    d->formatCategories = categories;
    const int maxCategory = Utils::maxElementOr(categories, {-1, C_TEXT}).first;
    d->formats = QList<QTextCharFormat>(maxCategory + 1);
    d->updateFormats(d->fontSettings);
}

QTextCharFormat SyntaxHighlighter::formatForCategory(int category) const
{
    QTC_ASSERT(d->formats.size() > category, return QTextCharFormat());

    return d->formats.at(category);
}

QTextCharFormat SyntaxHighlighter::whitespacified(const QTextCharFormat &fmt)
{
    QTextCharFormat format = d->whitespaceFormat;
    format.setBackground(fmt.background());
    return format;
}

QTextCharFormat SyntaxHighlighter::asSyntaxHighlight(const QTextCharFormat &fmt)
{
    QTextCharFormat format = fmt;
    format.setProperty(SyntaxHighlight, true);
    return format;
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    formatSpaces(text);
}

void SyntaxHighlighterPrivate::updateFormats(const FontSettings &fontSettings)
{
    this->fontSettings = fontSettings;
    // C_TEXT is handled by text editor's foreground and background color,
    // so use empty format for that
    for (const auto &pair : std::as_const(formatCategories)) {
        formats[pair.first] = pair.second == C_TEXT ? QTextCharFormat()
                                                    : fontSettings.toTextCharFormat(pair.second);
    }
    whitespaceFormat = fontSettings.toTextCharFormat(C_VISUAL_WHITESPACE);
}

} // namespace TextEditor
