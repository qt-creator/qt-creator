// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "syntaxhighlighter.h"
#include "textdocumentlayout.h"
#include "fontsettings.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/spellchecker.h>

#include <QElapsedTimer>
#include <QHash>
#include <QPointer>
#include <QTextDocument>
#include <QThread>

#include <cmath>

static Q_LOGGING_CATEGORY(Log, "qtc.editor.syntaxhighlighter", QtWarningMsg)

namespace TextEditor {

enum HighlighterTypeProperty
{
    SyntaxHighlight = QTextFormat::UserProperty + 1,
    SemanticHighlight = QTextFormat::UserProperty + 2,
    SpellingError = QTextFormat::UserProperty + 3
};

class SyntaxHighlighterPrivate
{
public:
    SyntaxHighlighterPrivate(SyntaxHighlighter *q) : q(q) {}
    SyntaxHighlighterPrivate(SyntaxHighlighter *q, const FontSettingsData &fontSettings)
        : SyntaxHighlighterPrivate(q)
    {
        updateFormats(fontSettings);
    }

    QPointer<QTextDocument> doc;

    void updateFormats(int from, int charsRemoved, int charsAdded);
    void reformatBlocks(int from, int charsRemoved, int charsAdded);
    void rehighlightBlocks(int from, int charsRemoved, int charsAdded);
    void reformatBlocks();
    void reformatBlock(const QTextBlock &block);

    inline void rehighlight(QTextCursor &cursor, QTextCursor::MoveOperation operation) {
        inReformatBlocks = true;
        int from = cursor.position();
        cursor.movePosition(operation);
        // The text itself did not change, so the formats of a block stay where they are.
        rehighlightBlocks(from, 0, cursor.position() - from);
        inReformatBlocks = false;
    }

    void applyFormatChanges();
    void updateFormats(const FontSettingsData &fontSettings);

    bool isBlockVisible(const QTextBlock &block) const;
    void rehighlightBlocks(const QTextBlock &first, const QTextBlock &last);
    std::pair<QTextBlock, QTextBlock> nextUncheckedVisibleRun() const;
    void rehighlightUncheckedVisibleBlocks();

    FontSettingsData fontSettings;
    QList<QTextCharFormat> formatChanges;
    QTextBlock currentBlock;
    bool rehighlightPending = false;
    bool inReformatBlocks = false;
    TextDocumentLayout::FoldValidator foldValidator;
    QList<QTextCharFormat> formats;
    QList<std::pair<int,TextStyle>> formatCategories;
    QTextCharFormat whitespaceFormat;
    QString mimeType;
    QString spellCheckLanguage;
    bool spellCheckStrings = false;
    QList<Utils::SpellChecker::Range> proseRanges;
    // The runs of blocks each viewer of the document shows, empty while no viewer said.
    QHash<QObject *, QList<std::pair<int, int>>> visibleBlocks;
    bool rehighlightUncheckedPending = false;
    bool currentBlockIsVisible = true;
    int spellCheckCursorPosition = -1;
    // The block that holds a mark back for the word the text cursor is on. A block
    // rather than its number, which shifts when a line above it goes in or out.
    QTextBlock spellCheckHeldBackBlock;
    bool syntaxInfoUpToDate = false;
    bool continueRehighlightScheduled = false;
    bool ignoreFolding = false;
    int highlightStartBlock = 0;
    int highlightEndBlock = 0;
    QSet<int> forceRehighlightBlocks;
    SyntaxHighlighter * const q;
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
SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent, const FontSettingsData &fontsettings)
    : QObject(parent), d(new SyntaxHighlighterPrivate(this, fontsettings))
{
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
                r.start += preeditText.size();
            } else if (r.start + r.length > preeditPosition) {
                QTextLayout::FormatRange beforePreeditRange = r;
                r.start = preeditPosition + preeditText.size();
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
    rehighlightBlocks(from, charsRemoved, charsAdded);
}

void SyntaxHighlighterPrivate::rehighlightBlocks(int from, int charsRemoved, int charsAdded)
{
    QTextBlock block = doc->findBlock(from);
    if (block.isValid() && block.blockNumber() < highlightStartBlock)
        highlightStartBlock = block.blockNumber();
    block = doc->findBlock(from + charsAdded + (charsRemoved > 0 ? 1 : 0));
    if (!block.isValid())
        highlightEndBlock = doc->blockCount() - 1;
    else if (block.blockNumber() > highlightEndBlock)
        highlightEndBlock = block.blockNumber();

    qCDebug(Log) << "reformat blocks from:" << from << "to:" << from + charsAdded - charsRemoved;

    if (!continueRehighlightScheduled)
        reformatBlocks();
}

void SyntaxHighlighterPrivate::reformatBlocks()
{
    QElapsedTimer et;
    et.start();

    continueRehighlightScheduled = false;
    syntaxInfoUpToDate = false;
    rehighlightPending = false;

    bool forceHighlightOfNextBlock = false;
    qCDebug(Log) << "continue reformat blocks start block:" << highlightStartBlock
                 << "end block:" << highlightEndBlock << "blockCount:" << doc->blockCount();
    QTextBlock block = doc->findBlockByNumber(highlightStartBlock);
    QTC_ASSERT(block.isValid(), block = doc->firstBlock());
    QTextBlock endBlock = doc->findBlockByNumber(highlightEndBlock);
    QTC_ASSERT(endBlock.isValid(), endBlock = doc->lastBlock());

    foldValidator.reset(block);

    while (block.isValid()) {
        highlightStartBlock = block.blockNumber();
        if (et.elapsed() > 20)
            break;

        const int stateBeforeHighlight = block.userState();
        const int braceDepthBeforeHighlight = TextBlockUserData::braceDepth(block);

        if (forceHighlightOfNextBlock || forceRehighlightBlocks.contains(block.blockNumber())
                || block.blockNumber() <= highlightEndBlock) {
            reformatBlock(block);
            forceRehighlightBlocks.remove(block.blockNumber());
            forceHighlightOfNextBlock = (block.userState() != stateBeforeHighlight)
                                        || (braceDepthBeforeHighlight
                                            != TextBlockUserData::braceDepth(block))
                                        || forceRehighlightBlocks.contains(block.blockNumber() + 1);
        }

        if (block == endBlock && !forceHighlightOfNextBlock)
            break;
        block = block.next();
    }

    formatChanges.clear();
    foldValidator.finalize();

    if (endBlock.isValid() && block.isValid() && block.blockNumber() < endBlock.blockNumber()) {
        continueRehighlightScheduled = true;
        QMetaObject::invokeMethod(q, &SyntaxHighlighter::continueRehighlight, Qt::QueuedConnection);
        if (forceHighlightOfNextBlock)
            forceRehighlightBlocks << block.blockNumber();
    } else {
        highlightEndBlock = 0;
        highlightStartBlock = INT_MAX;
        qCDebug(Log) << "reformat blocks done";
        syntaxInfoUpToDate = true;
        emit q->finished();
        if (rehighlightUncheckedPending) {
            rehighlightUncheckedPending = false;
            rehighlightUncheckedVisibleBlocks();
        }
    }
}

void SyntaxHighlighterPrivate::reformatBlock(const QTextBlock &block)
{
    QTC_ASSERT(!currentBlock.isValid(), return);

    currentBlock = block;
    currentBlockIsVisible = isBlockVisible(block);

    formatChanges.fill(QTextCharFormat(), block.length() - 1);
    proseRanges.clear();
    q->highlightBlock(block.text());
    applyFormatChanges();

    // What the block carries now was put there with the prose of a block in view
    // checked and the prose of one out of view left alone. Scrolling it into view is
    // what brings the marks of a misspelled word out.
    if (!spellCheckLanguage.isEmpty())
        TextBlockUserData::setSpellChecked(block, currentBlockIsVisible);

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
    : QObject(parent), d(new SyntaxHighlighterPrivate(this))
{
}

/*!
    Constructs a SyntaxHighlighter and installs it on \a parent.
    The specified QTextDocument also becomes the owner of the
    SyntaxHighlighter.
*/
SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QObject(parent), d(new SyntaxHighlighterPrivate(this))
{
    if (parent)
        setDocument(parent);
}

/*!
    Constructs a SyntaxHighlighter and installs it on \a parent's
    QTextDocument. The specified QTextEdit also becomes the owner of
    the SyntaxHighlighter.
*/
SyntaxHighlighter::SyntaxHighlighter(QTextEdit *parent)
    : QObject(parent), d(new SyntaxHighlighterPrivate(this))
{
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
    d->spellCheckHeldBackBlock = QTextBlock();
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
    const int end = std::min(start + count, int(text.size()));
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
    Marks the \a count characters at \a start of the current text block as prose, for
    spellCheck() to look at. A highlighter may call this unconditionally: without a
    language to check the prose in there is nothing to mark it for.

    \sa spellCheck()
*/
void SyntaxHighlighter::addProseRange(int start, int count)
{
    if (count <= 0 || d->spellCheckLanguage.isEmpty())
        return;

    // A word split over two ranges, as an escape sequence splits a string, is still
    // one word to the dictionary.
    if (!d->proseRanges.isEmpty()) {
        Utils::SpellChecker::Range &last = d->proseRanges.last();
        if (last.start + last.length == start) {
            last.length += count;
            return;
        }
    }
    d->proseRanges.append({start, count});
}

/*!
    Marks the misspelled words in the prose of the current text block with \a text with
    the spelling error format. The prose is what addProseRange() was called for, which is
    nothing at all unless a highlighter says otherwise.

    Only the prose of \a text reaches the dictionary. Whether a word is prose or a token
    of code is read off the whole of it either way: the characters next to a word tell,
    and the range it sits in does not.

    \sa addProseRange(), setSpellCheckLanguage()
*/
void SyntaxHighlighter::spellCheck(const QString &text)
{
    if (d->spellCheckHeldBackBlock == d->currentBlock)
        d->spellCheckHeldBackBlock = QTextBlock();

    if (d->spellCheckLanguage.isEmpty() || d->proseRanges.isEmpty() || !d->currentBlockIsVisible)
        return;

    // A mark the color scheme draws nothing for is no mark, and asking the dictionary
    // for one is work for nothing.
    const QTextCharFormat spellErrorFormat = d->fontSettings.toTextCharFormat(C_SPELL_ERROR);
    if (spellErrorFormat.underlineStyle() == QTextCharFormat::NoUnderline)
        return;

    const int blockPosition = d->currentBlock.position();
    const QList<Utils::SpellChecker::Range> ranges
        = Utils::SpellChecker::instance()->misspelledRanges(text,
                                                            d->proseRanges,
                                                            d->spellCheckLanguage);

    for (const Utils::SpellChecker::Range &range : ranges) {
        const int wordEnd = range.start + range.length;
        if (d->spellCheckCursorPosition >= blockPosition + range.start
            && d->spellCheckCursorPosition <= blockPosition + wordEnd) {
            // setSpellCheckCursorPosition() reads this back to tell whether moving the
            // cursor off this block brings a mark out.
            d->spellCheckHeldBackBlock = d->currentBlock;
            continue;
        }
        for (int i = range.start; i < wordEnd && i < d->formatChanges.size(); ++i) {
            QTextCharFormat &format = d->formatChanges[i];
            format.setUnderlineColor(spellErrorFormat.underlineColor());
            format.setUnderlineStyle(spellErrorFormat.underlineStyle());
            format.setProperty(SpellingError, true);
        }
    }
}

/*!
    Whether \a format is the one spellCheck() marks a misspelled word with. The
    underline style and color of the spelling error format are no answer on their own:
    a color scheme may give another category the same ones.
*/
bool SyntaxHighlighter::isSpellingError(const QTextCharFormat &format)
{
    return format.property(SpellingError).toBool();
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

    const int end = std::min(start + count, int(text.size()));
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

void SyntaxHighlighter::forceRehighlightBlock(const QTextBlock &block)
{
    QTC_ASSERT(block.isValid(), return);
    d->forceRehighlightBlocks << block.blockNumber();
}

void SyntaxHighlighter::setFoldingIndent(const QTextBlock &block, int indent)
{
    if (!ignoresFolding())
        TextBlockUserData::setFoldingIndent(block, indent);
}

void SyntaxHighlighter::setFoldingStartIncluded(const QTextBlock &block, bool included)
{
    if (!ignoresFolding())
        TextBlockUserData::setFoldingStartIncluded(block, included);
}

void SyntaxHighlighter::setFoldingEndIncluded(const QTextBlock &block, bool included)
{
    if (!ignoresFolding())
        TextBlockUserData::setFoldingEndIncluded(block, included);
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
                r.start += preeditText.size();
            } else if (r.start + r.length > preeditPosition) {
                QTextLayout::FormatRange afterPreeditRange = r;
                afterPreeditRange.start = preeditPosition + preeditText.size();
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

void SyntaxHighlighter::setIgnoreFolding(bool ignore)
{
    if (d->ignoreFolding == ignore)
        return;
    d->ignoreFolding = ignore;
    rehighlight();
}

bool SyntaxHighlighter::ignoresFolding() const
{
    return d->ignoreFolding;
}

void SyntaxHighlighter::setSpellCheckLanguage(const QString &language)
{
    if (d->spellCheckLanguage == language)
        return;
    d->spellCheckLanguage = language;
    if (!language.isEmpty()) {
        connect(Utils::SpellChecker::instance(), &Utils::SpellChecker::dictionaryChanged,
                this, &SyntaxHighlighter::scheduleRehighlight, Qt::UniqueConnection);
    }
    rehighlight();
}

QString SyntaxHighlighter::spellCheckLanguage() const
{
    return d->spellCheckLanguage;
}

void SyntaxHighlighter::setSpellCheckStrings(bool check)
{
    if (d->spellCheckStrings == check)
        return;
    d->spellCheckStrings = check;
    rehighlight();
}

bool SyntaxHighlighter::spellCheckStrings() const
{
    return d->spellCheckStrings;
}

// Whether block shows a misspelled word at position, the word whose mark the text
// cursor is to hold back.
static bool holdsSpellingErrorAt(const QTextBlock &block, int position)
{
    const QTextLayout *layout = block.layout();
    if (!layout)
        return false;

    const int offset = position - block.position();
    return Utils::anyOf(layout->formats(), [offset](const QTextLayout::FormatRange &range) {
        return SyntaxHighlighter::isSpellingError(range.format) && offset >= range.start
               && offset <= range.start + range.length;
    });
}

bool SyntaxHighlighterPrivate::isBlockVisible(const QTextBlock &block) const
{
    // A highlighter that no viewer registered with shows everything it has.
    if (visibleBlocks.isEmpty())
        return true;
    // A block a fold hides is in the viewport of no viewer, whatever range of block
    // numbers it falls into: a folded section spans numbers without taking up a row.
    if (!block.isVisible())
        return false;
    const int blockNumber = block.blockNumber();
    return Utils::anyOf(visibleBlocks, [blockNumber](const QList<std::pair<int, int>> &ranges) {
        return Utils::anyOf(ranges, [blockNumber](const std::pair<int, int> &range) {
            return blockNumber >= range.first && blockNumber <= range.second;
        });
    });
}

void SyntaxHighlighterPrivate::rehighlightBlocks(const QTextBlock &first, const QTextBlock &last)
{
    const bool wasPending = rehighlightPending;
    const bool wasInReformatBlocks = inReformatBlocks;
    const int from = first.position();
    const int to = last.position() + last.length() - 1;

    inReformatBlocks = true;
    // The text itself did not change, so the formats of a block stay where they are.
    rehighlightBlocks(from, 0, to - from);
    inReformatBlocks = wasInReformatBlocks;

    if (wasPending)
        rehighlightPending = true;
}

// A run of blocks that a viewer shows in one piece and the dictionary was not asked
// about, with an invalid first block when no such block is left. A block a fold hides
// ends a run: it takes up a number in the range a viewer reports without being shown.
std::pair<QTextBlock, QTextBlock> SyntaxHighlighterPrivate::nextUncheckedVisibleRun() const
{
    for (const QList<std::pair<int, int>> &ranges : visibleBlocks) {
        for (const std::pair<int, int> &range : ranges) {
            QTextBlock first;
            QTextBlock last;
            for (QTextBlock block = doc->findBlockByNumber(range.first);
                 block.isValid() && block.blockNumber() <= range.second;
                 block = block.next()) {
                if (block.isVisible() && !TextBlockUserData::spellChecked(block)) {
                    if (!first.isValid())
                        first = block;
                    last = block;
                } else if (first.isValid()) {
                    return {first, last};
                }
            }
            if (first.isValid())
                return {first, last};
        }
    }
    return {};
}

// The blocks a viewer scrolled into view, or a fold it opened, were highlighted with
// their prose left unchecked. Highlighting them again is what asks the dictionary
// about them. What the blocks in between were spared stays spared, which takes one
// run at a time: a rehighlight that ran out of its slice of the GUI thread widens the
// range it continues with to whatever it is asked for next, so the run after it waits
// until it is through rather than dragging the checked blocks of the gap along.
void SyntaxHighlighterPrivate::rehighlightUncheckedVisibleBlocks()
{
    if (spellCheckLanguage.isEmpty() || !doc)
        return;

    while (!continueRehighlightScheduled) {
        const std::pair<QTextBlock, QTextBlock> run = nextUncheckedVisibleRun();
        if (!run.first.isValid())
            return;
        rehighlightBlocks(run.first, run.second);
    }
    rehighlightUncheckedPending = true;
}

void SyntaxHighlighter::setVisibleBlocks(QObject *viewer, const QList<std::pair<int, int>> &ranges)
{
    QTC_ASSERT(viewer, return);

    const auto it = d->visibleBlocks.find(viewer);
    if (it != d->visibleBlocks.end()) {
        // Not skipped when the ranges are the ones the viewer had: opening a fold
        // brings blocks into view without moving either end of the range they fall in.
        *it = ranges;
    } else {
        d->visibleBlocks.insert(viewer, ranges);
        connect(viewer, &QObject::destroyed, this, [this, viewer] { removeViewer(viewer); });
    }
    d->rehighlightUncheckedVisibleBlocks();
}

void SyntaxHighlighter::removeViewer(QObject *viewer)
{
    QTC_ASSERT(viewer, return);

    // A viewer that is gone leaves behind the marks it asked for and asks for no block
    // it did not have before.
    d->visibleBlocks.remove(viewer);
}

void SyntaxHighlighter::setSpellCheckCursorPosition(int position)
{
    if (d->spellCheckCursorPosition == position)
        return;

    const int previousPosition = d->spellCheckCursorPosition;
    d->spellCheckCursorPosition = position;
    if (d->spellCheckLanguage.isEmpty() || !d->doc)
        return;

    // Rehighlighting a block runs the dictionary over it again, a call into the spell
    // checking service of the platform, and a text cursor moves on every key press.
    // Only a block whose marks the move changes needs it: the one that held a mark
    // back, and the one that is to hold one back now.
    const QTextBlock previousBlock = d->doc->findBlock(previousPosition);
    const QTextBlock currentBlock = d->doc->findBlock(position);
    const bool previousHeldBack = previousBlock.isValid()
                                  && previousBlock == d->spellCheckHeldBackBlock;
    if (previousHeldBack)
        rehighlightBlock(previousBlock);
    if (currentBlock.isValid() && !(previousHeldBack && currentBlock == previousBlock)
        && holdsSpellingErrorAt(currentBlock, position)) {
        rehighlightBlock(currentBlock);
    }
}

void SyntaxHighlighter::clearExtraFormats(const QTextBlock &block)
{
    const int blockLength = block.length();
    if (block.layout() == nullptr || blockLength == 0)
        return;

    const QList<QTextLayout::FormatRange> allFormats = block.layout()->formats();
    const QList<QTextLayout::FormatRange> formatsToApply
        = Utils::filtered(allFormats, [](const QTextLayout::FormatRange &r) {
              return !r.format.property(SemanticHighlight).toBool();
          });

    // Nothing was cleared: skip re-setting identical formats and the
    // markContentsDirty() below, which would otherwise force a relayout and
    // repaint of the block for no visible change. The incremental highlighter
    // clears every block between results, so most calls land here.
    if (formatsToApply.size() == allFormats.size())
        return;

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

void SyntaxHighlighter::setFontSettings(const FontSettingsData &fontSettings)
{
    d->updateFormats(fontSettings);
}

FontSettingsData SyntaxHighlighter::fontSettings() const
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

void SyntaxHighlighterPrivate::updateFormats(const FontSettingsData &fontSettings)
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
