// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include "textdocument.h"
#include "textmark.h"
#include "textsuggestion.h"

#include <utils/id.h>
#include <utils/plaintextedit/plaintextedit.h>

#include <KSyntaxHighlighting/State>

#include <QPlainTextDocumentLayout>
#include <QTextBlockUserData>

namespace TextEditor {

class TEXTEDITOR_EXPORT Parenthesis
{
public:
    enum Type : char { Opened, Closed };

    Parenthesis() = default;
    Parenthesis(Type t, QChar c, int position) : pos(position), chr(c), type(t) {}

    friend TEXTEDITOR_EXPORT QDebug operator<<(QDebug debug, const Parenthesis &parenthesis);

    int pos = -1;
    QChar chr;
    Utils::Id source;
    Type type = Opened;

    bool operator==(const Parenthesis &other) const;
};
using Parentheses = QList<Parenthesis>;
TEXTEDITOR_EXPORT void insertSorted(Parentheses &list, const Parenthesis &elem);

class TEXTEDITOR_EXPORT CodeFormatterData
{
public:
    virtual ~CodeFormatterData();
};

class TEXTEDITOR_EXPORT TextBlockUserData : public QTextBlockUserData
{
public:

    TextBlockUserData() = default;
    ~TextBlockUserData() override;

    TextMarks marks() const { return m_marks; }
    void addMark(TextMark *mark);
    bool removeMark(TextMark *mark) { return m_marks.removeAll(mark); }

    TextMarks documentClosing() {
        const TextMarks marks = m_marks;
        for (TextMark *mrk : marks)
            mrk->setBaseTextDocument(nullptr);
        m_marks.clear();
        return marks;
    }

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

    static void setParentheses(const QTextBlock &block, const Parentheses &parentheses);
    static void clearParentheses(const QTextBlock &block) { setParentheses(block, Parentheses());}
    static Parentheses parentheses(const QTextBlock &block);
    static bool hasParentheses(const QTextBlock &block);
    static void setIfdefedOut(const QTextBlock &block);
    static void clearIfdefedOut(const QTextBlock &block);
    static bool ifdefedOut(const QTextBlock &block);
    static int braceDepthDelta(const QTextBlock &block);
    static int braceDepth(const QTextBlock &block);
    static void setBraceDepth(const QTextBlock &block, int depth);
    static void changeBraceDepth(const QTextBlock &block, int delta);
    static void setFoldingIndent(const QTextBlock &block, int indent);
    static int foldingIndent(const QTextBlock &block);
    static void setLexerState(const QTextBlock &block, int state);
    static int lexerState(const QTextBlock &block);
    static void changeFoldingIndent(QTextBlock &block, int delta);
    static bool canFold(const QTextBlock &block);
    static void doFoldOrUnfold(const QTextBlock &block, bool unfold, bool recursive = false);
    static bool isFolded(const QTextBlock &block);
    static void setFolded(const QTextBlock &block, bool folded);
    static void setExpectedRawStringSuffix(const QTextBlock &block, const QByteArray &suffix);
    static QByteArray expectedRawStringSuffix(const QTextBlock &block);
    static TextSuggestion *suggestion(const QTextBlock &block);
    static void insertSuggestion(const QTextBlock &block, std::unique_ptr<TextSuggestion> &&suggestion);
    static void clearSuggestion(const QTextBlock &block);
    static void setAttributeState(const QTextBlock &block, quint8 attrState);
    static quint8 attributeState(const QTextBlock &block);
    static void updateSuggestionFormats(const QTextBlock &block, const FontSettings &fontSettings);
    static KSyntaxHighlighting::State syntaxState(const QTextBlock &block);
    static void setSyntaxState(const QTextBlock &block, KSyntaxHighlighting::State state);

    /* Set the code folding level.
     *
     * A code folding marker will appear the line *before* the one where the indention
     * level increases. The code folding reagion will end in the last line that has the same
     * indention level (or higher).
     */
    static void setFoldingStartIncluded(const QTextBlock &block, bool included);
    static bool foldingStartIncluded(const QTextBlock &block);
    // Set whether the last character of the folded region will show when the code is folded.
    static void setFoldingEndIncluded(const QTextBlock &block, bool included);
    static bool foldingEndIncluded(const QTextBlock &block);

    static CodeFormatterData *codeFormatterData(const QTextBlock &block);
    static void setCodeFormatterData(const QTextBlock &block, CodeFormatterData *data);

    static void addEmbeddedWidget(const QTextBlock &block, QWidget *widget);
    static void removeEmbeddedWidget(const QTextBlock &block, QWidget *widget);
    static QList<QPointer<QWidget>> embeddedWidgets(const QTextBlock &block);

    static void setAdditionalAnnotationHeight(const QTextBlock &block, int annotationHeight);
    static int additionalAnnotationHeight(const QTextBlock &block);

    static TextBlockUserData *textUserData(const QTextBlock &block) {
        return static_cast<TextBlockUserData*>(block.userData());
    }
    static TextBlockUserData *userData(const QTextBlock &block) {
        auto data = static_cast<TextBlockUserData*>(block.userData());
        if (!data && block.isValid())
            const_cast<QTextBlock &>(block).setUserData((data = new TextBlockUserData));
        return data;
    }

private:
    TextMarks m_marks;
    int m_foldingIndent : 16 = 0;
    int m_braceDepth : 16 = 0;
    int m_additionalAnnotationHeight : 16 = 0;
    int m_lexerState : 8 = 0;
    uint m_folded : 1 = false;
    uint m_ifdefedOut : 1 = false;
    uint m_foldingStartIncluded : 1 = false;
    uint m_foldingEndIncluded : 1 = false;
    Parentheses m_parentheses;
    CodeFormatterData *m_codeFormatterData = nullptr;
    KSyntaxHighlighting::State m_syntaxState;
    QByteArray m_expectedRawStringSuffix; // A bit C++-specific, but let's be pragmatic.
    std::unique_ptr<QTextDocument> m_replacement;
    std::unique_ptr<TextSuggestion> m_suggestion;
    QList<QPointer<QWidget>> m_embeddedWidgets;
    quint8 m_attrState = 0;
};

class TEXTEDITOR_EXPORT TextDocumentLayout : public Utils::PlainTextDocumentLayout
{
    Q_OBJECT

public:
    TextDocumentLayout(QTextDocument *doc);
    ~TextDocumentLayout() override;

    class TEXTEDITOR_EXPORT FoldValidator
    {
    public:
        void setup(TextDocumentLayout *layout);
        void reset();
        void process(QTextBlock block);
        void finalize();

    private:
        TextDocumentLayout *m_layout = nullptr;
        bool m_requestDocUpdate = false;
        int m_insideFold = 0;
    };

    void requestExtraAreaUpdate();

    void emitDocumentSizeChanged() { emit documentSizeChanged(documentSize()); }

    int lastSaveRevision = 0;
    bool hasMarks = false;
    bool hasLocationMarker = false;
    int m_requiredWidth = 0;

    void setRequiredWidth(int width);

    QSizeF documentSize() const override;
    QRectF blockBoundingRect(const QTextBlock &block) const override;

    TextMarks documentClosing();
    void documentAboutToReload(TextDocument *baseTextDocument);
    void documentReloaded(TextDocument *baseextDocument);
    void updateMarksLineNumber();
    void updateMarksBlock(const QTextBlock &block);
    void scheduleUpdate();
    void requestUpdateNow();

    int embeddedWidgetOffset(const QTextBlock &block, QWidget *widget);

private:
    bool m_updateScheduled = false;
    TextMarks m_reloadMarks;
    void resetReloadMarks();

signals:
    void updateExtraArea();
    void foldChanged(const int blockNumber, bool folded);
    void parenthesesChanged(const QTextBlock block);
};

} // namespace TextEditor
