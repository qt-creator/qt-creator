// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plaintexteditaccessibility.h"

#include "plaintextedit.h"

#include <QScrollBar>
#include <QTextBlock>

namespace Utils {

AccessiblePlainTextEdit::AccessiblePlainTextEdit(QWidget* o)
  : AccessibleTextWidget(o)
{
    Q_ASSERT(qobject_cast<PlainTextEdit *>(widget()));
}

PlainTextEdit* AccessiblePlainTextEdit::plainTextEdit() const
{
    return static_cast<PlainTextEdit *>(widget());
}

QString AccessiblePlainTextEdit::text(QAccessible::Text t) const
{
    if (t == QAccessible::Value)
        return plainTextEdit()->toPlainText();

    return QAccessibleWidget::text(t);
}

void AccessiblePlainTextEdit::setText(QAccessible::Text t, const QString &text)
{
    if (t != QAccessible::Value) {
        QAccessibleWidget::setText(t, text);
        return;
    }
    if (plainTextEdit()->isReadOnly())
        return;

    plainTextEdit()->setPlainText(text);
}

QAccessible::State AccessiblePlainTextEdit::state() const
{
    QAccessible::State st = AccessibleTextWidget::state();
    if (plainTextEdit()->isReadOnly())
        st.readOnly = true;
    else
        st.editable = true;
    return st;
}

void *AccessiblePlainTextEdit::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TextInterface)
        return static_cast<QAccessibleTextInterface*>(this);
    else if (t == QAccessible::EditableTextInterface)
        return static_cast<QAccessibleEditableTextInterface*>(this);
    return QAccessibleWidget::interface_cast(t);
}

QPoint AccessiblePlainTextEdit::scrollBarPosition() const
{
    QPoint result;
    result.setX(plainTextEdit()->horizontalScrollBar() ? plainTextEdit()->horizontalScrollBar()->sliderPosition() : 0);
    result.setY(plainTextEdit()->verticalScrollBar() ? plainTextEdit()->verticalScrollBar()->sliderPosition() : 0);
    return result;
}

QTextCursor AccessiblePlainTextEdit::textCursor() const
{
    return plainTextEdit()->textCursor();
}

void AccessiblePlainTextEdit::setTextCursor(const QTextCursor &textCursor)
{
    plainTextEdit()->setTextCursor(textCursor);
}

QTextDocument* AccessiblePlainTextEdit::textDocument() const
{
    return plainTextEdit()->document();
}

QWidget* AccessiblePlainTextEdit::viewport() const
{
    return plainTextEdit()->viewport();
}

void AccessiblePlainTextEdit::scrollToSubstring(int startIndex, int endIndex)
{
    //TODO: Not implemented
    Q_UNUSED(startIndex);
    Q_UNUSED(endIndex);
}

AccessibleTextWidget::AccessibleTextWidget(QWidget *o, QAccessible::Role r, const QString &name):
    QAccessibleWidget(o, r, name)
{

}

QAccessible::State AccessibleTextWidget::state() const
{
    QAccessible::State s = QAccessibleWidget::state();
    s.selectableText = true;
    s.multiLine = true;
    return s;
}

QRect AccessibleTextWidget::characterRect(int offset) const
{
    QTextBlock block = textDocument()->findBlock(offset);
    if (!block.isValid())
        return QRect();

    QTextLayout *layout = block.layout();
    QPointF layoutPosition = layout->position();
    int relativeOffset = offset - block.position();
    QTextLine line = layout->lineForTextPosition(relativeOffset);

    QRect r;

    if (line.isValid()) {
        qreal x = line.cursorToX(relativeOffset);

        QTextCharFormat format;
        QTextBlock::iterator iter = block.begin();
        if (iter.atEnd())
            format = block.charFormat();
        else {
            while (!iter.atEnd() && !iter.fragment().contains(offset))
                ++iter;
            if (iter.atEnd()) // newline should have same format as preceding character
                --iter;
            format = iter.fragment().charFormat();
        }

        QFontMetrics fm(format.font());
        const QString ch = text(offset, offset + 1);
        if (!ch.isEmpty()) {
            int w = fm.horizontalAdvance(ch);
            int h = fm.height();
            r = QRect(layoutPosition.x() + x, layoutPosition.y() + line.y() + line.ascent() + fm.descent() - h,
                      w, h);
            r.moveTo(viewport()->mapToGlobal(r.topLeft()));
        }
        r.translate(-scrollBarPosition());
    }

    return r;
}

int AccessibleTextWidget::offsetAtPoint(const QPoint &point) const
{
    QPoint p = viewport()->mapFromGlobal(point);
    // convert to document coordinates
    p += scrollBarPosition();
    return textDocument()->documentLayout()->hitTest(p, Qt::ExactHit);
}

int AccessibleTextWidget::selectionCount() const
{
    return textCursor().hasSelection() ? 1 : 0;
}

namespace {
/*!
    \internal
    \brief Helper class for AttributeFormatter

    This class is returned from AttributeFormatter's indexing operator to act
    as a proxy for the following assignment.

    It uses perfect forwarding in its assignment operator to amend the RHS
    with the formatting of the key, using QStringBuilder. Consequently, the
    RHS can be anything that QStringBuilder supports.
*/
class AttributeFormatterRef {
    QString &string;
    const char *key;
    friend class AttributeFormatter;
    AttributeFormatterRef(QString &string, const char *key) : string(string), key(key) {}
public:
    template <typename RHS>
    void operator=(RHS &&rhs)
    { string += QLatin1StringView(key) + u':' + std::forward<RHS>(rhs) + u';'; }
};

/*!
    \internal
    \brief Small string-builder class that supports a map-like API to serialize key-value pairs.
    \code
    AttributeFormatter attrs;
    attrs["foo"] = QLatinString("hello") + world + u'!';
    \endcode
    The key type is always \c{const char*}, and the right-hand-side can
    be any QStringBuilder expression.

    Breaking it down, this class provides the indexing operator, stores
    the key in an instance of, and then returns, AttributeFormatterRef,
    which is the class that provides the assignment part of the operation.
*/
class AttributeFormatter {
    QString string;
public:
    AttributeFormatterRef operator[](const char *key)
    { return AttributeFormatterRef(string, key); }

    QString toFormatted() const { return string; }
};
} // unnamed namespace

QString AccessibleTextWidget::attributes(int offset, int *startOffset, int *endOffset) const
{
    /* The list of attributes can be found at:
     http://linuxfoundation.org/collaborate/workgroups/accessibility/iaccessible2/textattributes
    */

    // IAccessible2 defines -1 as length and -2 as cursor position
    if (offset == -2)
        offset = cursorPosition();

    const int charCount = characterCount();

    // -1 doesn't make much sense here, but it's better to return something
    // screen readers may ask for text attributes at the cursor pos which may be equal to length
    if (offset == -1 || offset == charCount)
        offset = charCount - 1;

    if (offset < 0 || offset > charCount) {
        *startOffset = -1;
        *endOffset = -1;
        return QString();
    }


    QTextCursor cursor = textCursor();
    cursor.setPosition(offset);
    QTextBlock block = cursor.block();

    int blockStart = block.position();
    int blockEnd = blockStart + block.length();

    QTextBlock::iterator iter = block.begin();
    int lastFragmentIndex = blockStart;
    while (!iter.atEnd()) {
        QTextFragment f = iter.fragment();
        if (f.contains(offset))
            break;
        lastFragmentIndex = f.position() + f.length();
        ++iter;
    }

    QTextCharFormat charFormat;
    if (!iter.atEnd()) {
        QTextFragment fragment = iter.fragment();
        charFormat = fragment.charFormat();
        int pos = fragment.position();
        // text block and fragment may overlap, use the smallest common range
        *startOffset = qMax(pos, blockStart);
        *endOffset = qMin(pos + fragment.length(), blockEnd);
    } else {
        charFormat = block.charFormat();
        *startOffset = lastFragmentIndex;
        *endOffset = blockEnd;
    }
    Q_ASSERT(*startOffset <= offset);
    Q_ASSERT(*endOffset >= offset);

    QTextBlockFormat blockFormat = cursor.blockFormat();

    const QFont charFormatFont = charFormat.font();

    AttributeFormatter attrs;
    QString family = charFormatFont.families().value(0, QString());
    if (!family.isEmpty()) {
        family = family.replace(u'\\', "\\\\");
        family = family.replace(u':', "\\:");
        family = family.replace(u',', "\\,");
        family = family.replace(u'=', "\\=");
        family = family.replace(u';', "\\;");
        family = family.replace(u'\"', "\\\"");
        attrs["font-family"] = u'"' + family + u'"';
    }

    int fontSize = int(charFormatFont.pointSize());
    if (fontSize)
        attrs["font-size"] = QString::fromLatin1("%1pt").arg(fontSize);

    //Different weight values are not handled
    attrs["font-weight"] = QString::fromLatin1(charFormatFont.weight() > QFont::Normal ? "bold" : "normal");

    QFont::Style style = charFormatFont.style();
    attrs["font-style"] = QString::fromLatin1((style == QFont::StyleItalic) ? "italic" : ((style == QFont::StyleOblique) ? "oblique": "normal"));

    attrs["text-line-through-type"] = charFormatFont.strikeOut() ? "single" : "none";

    QTextCharFormat::UnderlineStyle underlineStyle = charFormat.underlineStyle();
    if (underlineStyle == QTextCharFormat::NoUnderline && charFormatFont.underline()) // underline could still be set in the default font
        underlineStyle = QTextCharFormat::SingleUnderline;
    QString underlineStyleValue;
    switch (underlineStyle) {
    case QTextCharFormat::NoUnderline:
        break;
    case QTextCharFormat::SingleUnderline:
        underlineStyleValue = QStringLiteral("solid");
        break;
    case QTextCharFormat::DashUnderline:
        underlineStyleValue = QStringLiteral("dash");
        break;
    case QTextCharFormat::DotLine:
        underlineStyleValue = QStringLiteral("dash");
        break;
    case QTextCharFormat::DashDotLine:
        underlineStyleValue = QStringLiteral("dot-dash");
        break;
    case QTextCharFormat::DashDotDotLine:
        underlineStyleValue = QStringLiteral("dot-dot-dash");
        break;
    case QTextCharFormat::WaveUnderline:
        underlineStyleValue = QStringLiteral("wave");
        break;
    case QTextCharFormat::SpellCheckUnderline:
        underlineStyleValue = QStringLiteral("wave"); // this is not correct, but provides good approximation at least
        break;
    default:
        qWarning() << "Unknown QTextCharFormat::UnderlineStyle value " << underlineStyle << " could not be translated to IAccessible2 value";
        break;
    }
    if (!underlineStyleValue.isNull()) {
        attrs["text-underline-style"] = underlineStyleValue;
        attrs["text-underline-type"] = QStringLiteral("single"); // if underlineStyleValue is set, there is an underline, and Qt does not support other than single ones
    } // else both are "none" which is the default - no need to set them

    if (block.textDirection() == Qt::RightToLeft)
        attrs["writing-mode"] = QStringLiteral("rl");

    QTextCharFormat::VerticalAlignment alignment = charFormat.verticalAlignment();
    attrs["text-position"] = QString::fromLatin1((alignment == QTextCharFormat::AlignSubScript) ? "sub" : ((alignment == QTextCharFormat::AlignSuperScript) ? "super" : "baseline" ));

    QBrush background = charFormat.background();
    if (background.style() == Qt::SolidPattern) {
        attrs["background-color"] = QString::fromLatin1("rgb(%1,%2,%3)").arg(background.color().red()).arg(background.color().green()).arg(background.color().blue());
    }

    QBrush foreground = charFormat.foreground();
    if (foreground.style() == Qt::SolidPattern) {
        attrs["color"] = QString::fromLatin1("rgb(%1,%2,%3)").arg(foreground.color().red()).arg(foreground.color().green()).arg(foreground.color().blue());
    }

    switch (blockFormat.alignment() & (Qt::AlignLeft | Qt::AlignRight | Qt::AlignHCenter | Qt::AlignJustify)) {
    case Qt::AlignLeft:
        attrs["text-align"] = QStringLiteral("left");
        break;
    case Qt::AlignRight:
        attrs["text-align"] = QStringLiteral("right");
        break;
    case Qt::AlignHCenter:
        attrs["text-align"] = QStringLiteral("center");
        break;
    case Qt::AlignJustify:
        attrs["text-align"] = QStringLiteral("justify");
        break;
    }

    return attrs.toFormatted();
}

int AccessibleTextWidget::cursorPosition() const
{
    return textCursor().position();
}

void AccessibleTextWidget::selection(int selectionIndex, int *startOffset, int *endOffset) const
{
    *startOffset = *endOffset = 0;
    QTextCursor cursor = textCursor();

    if (selectionIndex != 0 || !cursor.hasSelection())
        return;

    *startOffset = cursor.selectionStart();
    *endOffset = cursor.selectionEnd();
}

QString AccessibleTextWidget::text(int startOffset, int endOffset) const
{
    QTextCursor cursor(textCursor());

    cursor.setPosition(startOffset, QTextCursor::MoveAnchor);
    cursor.setPosition(endOffset, QTextCursor::KeepAnchor);

    return cursor.selectedText().replace(QChar(QChar::ParagraphSeparator), u'\n');
}

QPoint AccessibleTextWidget::scrollBarPosition() const
{
    return QPoint(0, 0);
}

QString AccessibleTextWidget::textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                                int *startOffset, int *endOffset) const
{
    Q_ASSERT(startOffset);
    Q_ASSERT(endOffset);

    QTextCursor cursor = textCursor();
    cursor.setPosition(offset);
    std::pair<int, int> boundaries = QAccessible::qAccessibleTextBoundaryHelper(cursor, boundaryType);
    cursor.setPosition(boundaries.first - 1);
    boundaries = QAccessible::qAccessibleTextBoundaryHelper(cursor, boundaryType);

    *startOffset = boundaries.first;
    *endOffset = boundaries.second;

    return text(boundaries.first, boundaries.second);
}


QString AccessibleTextWidget::textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                               int *startOffset, int *endOffset) const
{
    Q_ASSERT(startOffset);
    Q_ASSERT(endOffset);

    QTextCursor cursor = textCursor();
    cursor.setPosition(offset);
    std::pair<int, int> boundaries = QAccessible::qAccessibleTextBoundaryHelper(cursor, boundaryType);
    cursor.setPosition(boundaries.second);
    boundaries = QAccessible::qAccessibleTextBoundaryHelper(cursor, boundaryType);

    *startOffset = boundaries.first;
    *endOffset = boundaries.second;

    return text(boundaries.first, boundaries.second);
}

QString AccessibleTextWidget::textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                            int *startOffset, int *endOffset) const
{
    Q_ASSERT(startOffset);
    Q_ASSERT(endOffset);

    QTextCursor cursor = textCursor();
    cursor.setPosition(offset);
    std::pair<int, int> boundaries = QAccessible::qAccessibleTextBoundaryHelper(cursor, boundaryType);

    *startOffset = boundaries.first;
    *endOffset = boundaries.second;

    return text(boundaries.first, boundaries.second);
}

void AccessibleTextWidget::setCursorPosition(int position)
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(position);
    setTextCursor(cursor);
}

void AccessibleTextWidget::addSelection(int startOffset, int endOffset)
{
    setSelection(0, startOffset, endOffset);
}

void AccessibleTextWidget::removeSelection(int selectionIndex)
{
    if (selectionIndex != 0)
        return;

    QTextCursor cursor = textCursor();
    cursor.clearSelection();
    setTextCursor(cursor);
}

void AccessibleTextWidget::setSelection(int selectionIndex, int startOffset, int endOffset)
{
    if (selectionIndex != 0)
        return;

    QTextCursor cursor = textCursor();
    cursor.setPosition(startOffset, QTextCursor::MoveAnchor);
    cursor.setPosition(endOffset, QTextCursor::KeepAnchor);
    setTextCursor(cursor);
}

int AccessibleTextWidget::characterCount() const
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    return cursor.position();
}

QTextCursor AccessibleTextWidget::textCursorForRange(int startOffset, int endOffset) const
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(startOffset, QTextCursor::MoveAnchor);
    cursor.setPosition(endOffset, QTextCursor::KeepAnchor);

    return cursor;
}

void AccessibleTextWidget::deleteText(int startOffset, int endOffset)
{
    QTextCursor cursor = textCursorForRange(startOffset, endOffset);
    cursor.removeSelectedText();
}

void AccessibleTextWidget::insertText(int offset, const QString &text)
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(offset);
    cursor.insertText(text);
}

void AccessibleTextWidget::replaceText(int startOffset, int endOffset, const QString &text)
{
    QTextCursor cursor = textCursorForRange(startOffset, endOffset);
    cursor.removeSelectedText();
    cursor.insertText(text);
}

QAccessibleInterface *accessiblePlainTextEditFactory(const QString &classname, QObject *object)
{
    if (object && object->isWidgetType() && classname == "Utils::PlainTextEdit")
        return new AccessiblePlainTextEdit(static_cast<QWidget *>(object));
    return nullptr;
}

} // namespace Utils
