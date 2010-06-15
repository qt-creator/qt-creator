/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef BASETEXTDOCUMENTLAYOUT_H
#define BASETEXTDOCUMENTLAYOUT_H

#include "texteditor_global.h"

#include "itexteditor.h"

#include <QtGui/QTextBlockUserData>
#include <QtGui/QPlainTextDocumentLayout>

namespace TextEditor {

struct Parenthesis;
typedef QVector<Parenthesis> Parentheses;

struct TEXTEDITOR_EXPORT Parenthesis
{
    enum Type { Opened, Closed };

    inline Parenthesis() : type(Opened), pos(-1) {}
    inline Parenthesis(Type t, QChar c, int position)
        : type(t), chr(c), pos(position) {}
    Type type;
    QChar chr;
    int pos;
    static int collapseAtPos(const Parentheses &parentheses, QChar *character = 0);
    static int closeCollapseAtPos(const Parentheses &parentheses, QChar *character = 0);
    static bool hasClosingCollapse(const Parentheses &parentheses);
};


class TEXTEDITOR_EXPORT TextBlockUserData : public QTextBlockUserData
{
public:

    enum CollapseMode { NoCollapse , CollapseThis, CollapseAfter };
    enum ClosingCollapseMode { NoClosingCollapse, ClosingCollapse, ClosingCollapseAtEnd };

    inline TextBlockUserData()
        : m_collapseIncludesClosure(false),
          m_collapseMode(NoCollapse),
          m_closingCollapseMode(NoClosingCollapse),
          m_collapsed(false),
          m_ifdefedOut(false) {}
    ~TextBlockUserData();

    inline TextMarks marks() const { return m_marks; }
    inline void addMark(ITextMark *mark) { m_marks += mark; }
    inline bool removeMark(ITextMark *mark) { return m_marks.removeAll(mark); }
    inline bool hasMark(ITextMark *mark) const { return m_marks.contains(mark); }
    inline void clearMarks() { m_marks.clear(); }
    inline void documentClosing() { Q_FOREACH(ITextMark *tm, m_marks) { tm->documentClosing(); } m_marks.clear();}

    inline CollapseMode collapseMode() const { return (CollapseMode)m_collapseMode; }
    inline void setCollapseMode(CollapseMode c) { m_collapseMode = c; }

    inline void setClosingCollapseMode(ClosingCollapseMode c) { m_closingCollapseMode = c; }
    inline ClosingCollapseMode closingCollapseMode() const { return (ClosingCollapseMode) m_closingCollapseMode; }

    inline bool hasClosingCollapse() const { return closingCollapseMode() != NoClosingCollapse; }
    inline bool hasClosingCollapseAtEnd() const { return closingCollapseMode() == ClosingCollapseAtEnd; }
    inline bool hasClosingCollapseInside() const { return closingCollapseMode() == ClosingCollapse; }

    inline void setCollapsed(bool b) { m_collapsed = b; }
    inline bool collapsed() const { return m_collapsed; }

    inline void setCollapseIncludesClosure(bool b) { m_collapseIncludesClosure = b; }
    inline bool collapseIncludesClosure() const { return m_collapseIncludesClosure; }

    inline void setParentheses(const Parentheses &parentheses) { m_parentheses = parentheses; }
    inline void clearParentheses() { m_parentheses.clear(); }
    inline const Parentheses &parentheses() const { return m_parentheses; }
    inline bool hasParentheses() const { return !m_parentheses.isEmpty(); }
    int braceDepthDelta() const;

    inline bool setIfdefedOut() { bool result = m_ifdefedOut; m_ifdefedOut = true; return !result; }
    inline bool clearIfdefedOut() { bool result = m_ifdefedOut; m_ifdefedOut = false; return result;}
    inline bool ifdefedOut() const { return m_ifdefedOut; }

    inline static TextBlockUserData *canCollapse(const QTextBlock &block) {
        TextBlockUserData *data = static_cast<TextBlockUserData*>(block.userData());
        if (!data || data->collapseMode() != CollapseAfter) {
            data = static_cast<TextBlockUserData*>(block.next().userData());
            if (!data || data->collapseMode() != TextBlockUserData::CollapseThis)
                data = 0;
        }
        if (data && data->m_ifdefedOut)
            data = 0;
        return data;
    }

    inline static bool hasCollapseAfter(const QTextBlock & block)
    {
        if (!block.isValid()) {
            return false;
        } else if (block.next().isValid()) {
            TextBlockUserData *data = static_cast<TextBlockUserData*>(block.next().userData());
            if (data && data->collapseMode() == TextBlockUserData::CollapseThis &&  !data->m_ifdefedOut)
                return true;
        }
        return false;
    }

    inline static bool hasClosingCollapse(const QTextBlock &block) {
        TextBlockUserData *data = static_cast<TextBlockUserData*>(block.userData());
        return (data && data->hasClosingCollapse());
    }

    inline static bool hasClosingCollapseAtEnd(const QTextBlock &block) {
        TextBlockUserData *data = static_cast<TextBlockUserData*>(block.userData());
        return (data && data->hasClosingCollapseAtEnd());
    }

    inline static bool hasClosingCollapseInside(const QTextBlock &block) {
        TextBlockUserData *data = static_cast<TextBlockUserData*>(block.userData());
        return (data && data->hasClosingCollapseInside());
    }

    static QTextBlock testCollapse(const QTextBlock& block);
    static void doCollapse(const QTextBlock& block, bool visible);

    int collapseAtPos(QChar *character = 0) const;

    enum MatchType { NoMatch, Match, Mismatch  };
    static MatchType checkOpenParenthesis(QTextCursor *cursor, QChar c);
    static MatchType checkClosedParenthesis(QTextCursor *cursor, QChar c);
    static MatchType matchCursorBackward(QTextCursor *cursor);
    static MatchType matchCursorForward(QTextCursor *cursor);
    static bool findPreviousOpenParenthesis(QTextCursor *cursor, bool select = false);
    static bool findNextClosingParenthesis(QTextCursor *cursor, bool select = false);

    static bool findPreviousBlockOpenParenthesis(QTextCursor *cursor, bool checkStartPosition = false);
    static bool findNextBlockClosingParenthesis(QTextCursor *cursor);


private:
    TextMarks m_marks;
    uint m_collapseIncludesClosure : 1;
    uint m_collapseMode : 4;
    uint m_closingCollapseMode : 4;
    uint m_collapsed : 1;
    uint m_ifdefedOut : 1;
    Parentheses m_parentheses;
};


class TEXTEDITOR_EXPORT BaseTextDocumentLayout : public QPlainTextDocumentLayout
{
    Q_OBJECT

public:
    BaseTextDocumentLayout(QTextDocument *doc);
    ~BaseTextDocumentLayout();

    static void setParentheses(const QTextBlock &block, const Parentheses &parentheses);
    static void clearParentheses(const QTextBlock &block) { setParentheses(block, Parentheses());}
    static Parentheses parentheses(const QTextBlock &block);
    static bool hasParentheses(const QTextBlock &block);
    static bool setIfdefedOut(const QTextBlock &block);
    static bool clearIfdefedOut(const QTextBlock &block);
    static bool ifdefedOut(const QTextBlock &block);
    static int braceDepthDelta(const QTextBlock &block);
    static int braceDepth(const QTextBlock &block);
    static void setBraceDepth(QTextBlock &block, int depth);
    static void changeBraceDepth(QTextBlock &block, int delta);

    static TextBlockUserData *testUserData(const QTextBlock &block) {
        return static_cast<TextBlockUserData*>(block.userData());
    }
    static TextBlockUserData *userData(const QTextBlock &block) {
        TextBlockUserData *data = static_cast<TextBlockUserData*>(block.userData());
        if (!data && block.isValid())
            const_cast<QTextBlock &>(block).setUserData((data = new TextBlockUserData));
        return data;
    }


    void emitDocumentSizeChanged() { emit documentSizeChanged(documentSize()); }
    int lastSaveRevision;
    bool hasMarks;
};

} // namespace TextEditor

#endif // BASETEXTDOCUMENTLAYOUT_H
