/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BASETEXTDOCUMENTLAYOUT_H
#define BASETEXTDOCUMENTLAYOUT_H

#include "texteditor_global.h"

#include "itexteditor.h"

#include <QTextBlockUserData>
#include <QPlainTextDocumentLayout>

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
};

class TEXTEDITOR_EXPORT CodeFormatterData
{
public:
    virtual ~CodeFormatterData();
};

class TEXTEDITOR_EXPORT TextBlockUserData : public QTextBlockUserData
{
public:

    inline TextBlockUserData()
        : m_folded(false),
          m_ifdefedOut(false),
          m_foldingIndent(0),
          m_lexerState(0),
          m_foldingStartIncluded(false),
          m_foldingEndIncluded(false),
          m_codeFormatterData(0)
    {}
    ~TextBlockUserData();

    inline TextMarks marks() const { return m_marks; }
    void addMark(ITextMark *mark);
    inline bool removeMark(ITextMark *mark) { return m_marks.removeAll(mark); }

    inline TextMarks documentClosing() {
        TextMarks marks = m_marks;
        foreach (ITextMark *mrk, m_marks)
            mrk->setMarkableInterface(0);
        m_marks.clear();
        return marks;
    }

    inline void setFolded(bool b) { m_folded = b; }
    inline bool folded() const { return m_folded; }

    inline void setParentheses(const Parentheses &parentheses) { m_parentheses = parentheses; }
    inline void clearParentheses() { m_parentheses.clear(); }
    inline const Parentheses &parentheses() const { return m_parentheses; }
    inline bool hasParentheses() const { return !m_parentheses.isEmpty(); }
    int braceDepthDelta() const;

    inline bool setIfdefedOut() { bool result = m_ifdefedOut; m_ifdefedOut = true; return !result; }
    inline bool clearIfdefedOut() { bool result = m_ifdefedOut; m_ifdefedOut = false; return result;}
    inline bool ifdefedOut() const { return m_ifdefedOut; }


    enum MatchType { NoMatch, Match, Mismatch  };
    static MatchType checkOpenParenthesis(QTextCursor *cursor, QChar c);
    static MatchType checkClosedParenthesis(QTextCursor *cursor, QChar c);
    static MatchType matchCursorBackward(QTextCursor *cursor);
    static MatchType matchCursorForward(QTextCursor *cursor);
    static bool findPreviousOpenParenthesis(QTextCursor *cursor, bool select = false,
                                            bool onlyInCurrentBlock = false);
    static bool findNextClosingParenthesis(QTextCursor *cursor, bool select = false);

    static bool findPreviousBlockOpenParenthesis(QTextCursor *cursor, bool checkStartPosition = false);
    static bool findNextBlockClosingParenthesis(QTextCursor *cursor);

    // Get the code folding level
    inline int foldingIndent() const { return m_foldingIndent; }
    /* Set the code folding level.
     *
     * A code folding marker will appear the line *before* the one where the indention
     * level increases. The code folding reagion will end in the last line that has the same
     * indention level (or higher).
     */
    inline void setFoldingIndent(int indent) { m_foldingIndent = indent; }
    // Set whether the first character of the folded region will show when the code is folded.
    inline void setFoldingStartIncluded(bool included) { m_foldingStartIncluded = included; }
    inline bool foldingStartIncluded() const { return m_foldingStartIncluded; }
    // Set whether the last character of the folded region will show when the code is folded.
    inline void setFoldingEndIncluded(bool included) { m_foldingEndIncluded = included; }
    inline bool foldingEndIncluded() const { return m_foldingEndIncluded; }
    inline int lexerState() const { return m_lexerState; }
    inline void setLexerState(int state) {m_lexerState = state; }


    CodeFormatterData *codeFormatterData() const { return m_codeFormatterData; }
    void setCodeFormatterData(CodeFormatterData *data);

private:
    TextMarks m_marks;
    uint m_folded : 1;
    uint m_ifdefedOut : 1;
    uint m_foldingIndent : 16;
    uint m_lexerState : 4;
    uint m_foldingStartIncluded : 1;
    uint m_foldingEndIncluded : 1;
    Parentheses m_parentheses;
    CodeFormatterData *m_codeFormatterData;
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
    static void setFoldingIndent(const QTextBlock &block, int indent);
    static int foldingIndent(const QTextBlock &block);
    static void setLexerState(const QTextBlock &block, int state);
    static int lexerState(const QTextBlock &block);
    static void changeFoldingIndent(QTextBlock &block, int delta);
    static bool canFold(const QTextBlock &block);
    static void doFoldOrUnfold(const QTextBlock& block, bool unfold);
    static bool isFolded(const QTextBlock &block);
    static void setFolded(const QTextBlock &block, bool folded);

    class TEXTEDITOR_EXPORT FoldValidator
    {
    public:
        FoldValidator();

        void setup(BaseTextDocumentLayout *layout);
        void reset();
        void process(QTextBlock block);
        void finalize();

    private:
        BaseTextDocumentLayout *m_layout;
        bool m_requestDocUpdate;
        int m_insideFold;
    };

    static TextBlockUserData *testUserData(const QTextBlock &block) {
        return static_cast<TextBlockUserData*>(block.userData());
    }
    static TextBlockUserData *userData(const QTextBlock &block) {
        TextBlockUserData *data = static_cast<TextBlockUserData*>(block.userData());
        if (!data && block.isValid())
            const_cast<QTextBlock &>(block).setUserData((data = new TextBlockUserData));
        return data;
    }

    void requestExtraAreaUpdate();

    void emitDocumentSizeChanged() { emit documentSizeChanged(documentSize()); }
    ITextMarkable *markableInterface();

    int lastSaveRevision;
    bool hasMarks;
    double maxMarkWidthFactor;

    int m_requiredWidth;
    ITextMarkable *m_documentMarker;

    void setRequiredWidth(int width);

    QSizeF documentSize() const;

    TextMarks documentClosing();
    void documentReloaded(TextMarks marks);
    void updateMarksLineNumber();
    void updateMarksBlock(const QTextBlock &block);

signals:
    void updateExtraArea();
};

} // namespace TextEditor

#endif // BASETEXTDOCUMENTLAYOUT_H
