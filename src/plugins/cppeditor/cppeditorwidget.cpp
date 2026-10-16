// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppeditorwidget.h"
#include "cppparsecontext.h"

#include "cppcodeformatter.h"
#include "cppcompletionassistprovider.h"
#include "cppeditorconstants.h"
#include "cppeditordocument.h"
#include "cppeditoroutline.h"
#include "cppeditortr.h"
#include "cppfunctiondecldeflink.h"
#include "cppfunctionparamrenaminghandler.h"
#include "cpplocalrenaming.h"
#include "cppmodelmanager.h"
#include "cpppreprocessordialog.h"
#include "cppselectionchanger.h"
#include "cppsemanticinfo.h"
#include "cppuseselectionsupdater.h"
#include "doxygengenerator.h"
#include "quickfixes/cppquickfixassistant.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/extracompiler.h>
#include <projectexplorer/projectcommentssettings.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <texteditor/basefilefind.h>
#include <texteditor/behaviorsettings.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/commentssettings.h>
#include <texteditor/completionsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/refactoroverlay.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <cplusplus/ASTPath.h>
#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/MatchingText.h>

#include <utils/algorithm.h>
#include <utils/infobar.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/textutils.h>

#include <QAction>
#include <QApplication>
#include <QElapsedTimer>
#include <QMainWindow>
#include <QMenu>
#include <QPointer>
#include <QTextBlock>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QWidgetAction>

enum { UPDATE_FUNCTION_DECL_DEF_LINK_INTERVAL = 200 };

using namespace Core;
using namespace CPlusPlus;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor {
namespace Internal {

namespace {

bool isStartOfDoxygenComment(const QTextCursor &cursor)
{
    const int pos = cursor.position();

    QTextDocument *document = cursor.document();
    QString comment = QString(document->characterAt(pos - 3))
            + document->characterAt(pos - 2)
            + document->characterAt(pos - 1);

    return comment == QLatin1String("/**")
           || comment == QLatin1String("/*!")
           || comment == QLatin1String("///")
           || comment == QLatin1String("//!");
}

DoxygenGenerator::DocumentationStyle doxygenStyle(const QTextCursor &cursor,
                                                  const QTextDocument *doc)
{
    const int pos = cursor.position();

    QString comment = QString(doc->characterAt(pos - 3))
            + doc->characterAt(pos - 2)
            + doc->characterAt(pos - 1);

    if (comment == QLatin1String("/**"))
        return DoxygenGenerator::JavaStyle;
    else if (comment == QLatin1String("/*!"))
        return DoxygenGenerator::QtStyle;
    else if (comment == QLatin1String("///"))
        return DoxygenGenerator::CppStyleA;
    else
        return DoxygenGenerator::CppStyleB;
}

/// Check if previous line is a CppStyle Doxygen Comment
bool isPreviousLineCppStyleComment(const QTextCursor &cursor)
{
    const QTextBlock &currentBlock = cursor.block();
    if (!currentBlock.isValid())
        return false;

    const QTextBlock &actual = currentBlock.previous();
    if (!actual.isValid())
        return false;

    const QString text = actual.text().trimmed();
    return text.startsWith(QLatin1String("///")) || text.startsWith(QLatin1String("//!"));
}

/// Check if next line is a CppStyle Doxygen Comment
bool isNextLineCppStyleComment(const QTextCursor &cursor)
{
    const QTextBlock &currentBlock = cursor.block();
    if (!currentBlock.isValid())
        return false;

    const QTextBlock &actual = currentBlock.next();
    if (!actual.isValid())
        return false;

    const QString text = actual.text().trimmed();
    return text.startsWith(QLatin1String("///")) || text.startsWith(QLatin1String("//!"));
}

bool isCppStyleContinuation(const QTextCursor& cursor)
{
    return isPreviousLineCppStyleComment(cursor) || isNextLineCppStyleComment(cursor);
}

bool lineStartsWithCppDoxygenCommentAndCursorIsAfter(const QTextCursor &cursor,
                                                     const QTextDocument *doc)
{
    QTextCursor cursorFirstNonBlank(cursor);
    cursorFirstNonBlank.movePosition(QTextCursor::StartOfLine);
    while (doc->characterAt(cursorFirstNonBlank.position()).isSpace()
           && cursorFirstNonBlank.movePosition(QTextCursor::NextCharacter)) {
    }

    const QTextBlock& block = cursorFirstNonBlank.block();
    const QString text = block.text().trimmed();
    if (text.startsWith(QLatin1String("///")) || text.startsWith(QLatin1String("//!")))
        return (cursor.position() >= cursorFirstNonBlank.position() + 3);

    return false;
}

bool isCursorAfterNonNestedCppStyleComment(const QTextCursor &cursor,
                                           TextEditor::TextEditorWidget *editorWidget)
{
    QTextDocument *document = editorWidget->document();
    QTextCursor cursorBeforeCppComment(cursor);
    while (document->characterAt(cursorBeforeCppComment.position()) != QLatin1Char('/')
           && cursorBeforeCppComment.movePosition(QTextCursor::PreviousCharacter)) {
    }

    if (!cursorBeforeCppComment.movePosition(QTextCursor::PreviousCharacter))
        return false;

    if (document->characterAt(cursorBeforeCppComment.position()) != QLatin1Char('/'))
        return false;

    if (!cursorBeforeCppComment.movePosition(QTextCursor::PreviousCharacter))
        return false;

    return !CPlusPlus::MatchingText::isInCommentHelper(cursorBeforeCppComment);
}

bool handleDoxygenCppStyleContinuation(QTextCursor &cursor)
{
    const int blockPos = cursor.positionInBlock();
    const QString &text = cursor.block().text();
    int offset = 0;
    for (; offset < blockPos; ++offset) {
        if (!text.at(offset).isSpace())
            break;
    }

    // If the line does not start with the comment we don't
    // consider it as a continuation. Handles situations like:
    // void d(); ///<enter>
    if (offset + 3 > text.size())
        return false;
    const QStringView commentMarker = QStringView(text).mid(offset, 3);
    if (commentMarker != QLatin1String("///") && commentMarker != QLatin1String("//!"))
        return false;

    QString newLine(QLatin1Char('\n'));
    newLine.append(text.left(offset)); // indent correctly
    newLine.append(commentMarker.toString());
    newLine.append(QLatin1Char(' '));

    cursor.insertText(newLine);
    return true;
}

bool handleDoxygenContinuation(QTextCursor &cursor,
                               TextEditor::TextEditorWidget *editorWidget,
                               const bool enableDoxygen,
                               const bool leadingAsterisks)
{
    const QTextDocument *doc = editorWidget->document();

    // It might be a continuation if:
    // a) current line starts with /// or //! and cursor is positioned after the comment
    // b) current line is in the middle of a multi-line Qt or Java style comment

    if (!cursor.atEnd()) {
        if (enableDoxygen && lineStartsWithCppDoxygenCommentAndCursorIsAfter(cursor, doc))
            return handleDoxygenCppStyleContinuation(cursor);

        if (isCursorAfterNonNestedCppStyleComment(cursor, editorWidget))
            return false;
    }

    // We continue the comment if the cursor is after a comment's line asterisk and if
    // there's no asterisk immediately after the cursor (that would already be considered
    // a leading asterisk).
    int offset = 0;
    const int blockPos = cursor.positionInBlock();
    const QString &currentLine = cursor.block().text();
    for (; offset < blockPos; ++offset) {
        if (!currentLine.at(offset).isSpace())
            break;
    }

    // In case we don't need to insert leading asteriskses, this code will be run once (right after
    // hitting enter on the line containing '/*'). It will insert a continuation without an
    // asterisk, but with an extra space. After that, the normal indenting will take over and do the
    // Right Thing <TM>.
    if (offset < blockPos
            && (currentLine.at(offset) == QLatin1Char('*')
                || (offset < blockPos - 1
                    && currentLine.at(offset) == QLatin1Char('/')
                    && currentLine.at(offset + 1) == QLatin1Char('*')))) {
        // Ok, so the line started with an '*' or '/*'
        int followinPos = blockPos;
        // Now search for the first non-whitespace character to align to:
        for (; followinPos < currentLine.size(); ++followinPos) {
            if (!currentLine.at(followinPos).isSpace())
                break;
        }
        QString newLine(QLatin1Char('\n'));
        QTextCursor c(cursor);
        c.movePosition(QTextCursor::StartOfBlock);
        c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, offset);
        newLine.append(c.selectedText());
        const bool isAtAsterisk = followinPos < currentLine.size()
                                  && currentLine.at(followinPos) == '*';
        if (currentLine.at(offset) == QLatin1Char('/')) {
            if (leadingAsterisks) {
                if (isAtAsterisk)
                    newLine.append(" ");
                else
                    newLine.append(QLatin1String(" * "));
            } else {
                newLine.append(QLatin1String("   "));
            }
            offset += 3;
        } else {
            // If '*' is not within a comment, skip.
            QTextCursor cursorOnFirstNonWhiteSpace(cursor);
            const int positionOnFirstNonWhiteSpace = cursor.position() - blockPos + offset;
            cursorOnFirstNonWhiteSpace.setPosition(positionOnFirstNonWhiteSpace);
            if (!CPlusPlus::MatchingText::isInCommentHelper(cursorOnFirstNonWhiteSpace))
                return false;

            // ...otherwise do the continuation
            if (!isAtAsterisk) {
                int start = offset;
                while (offset < blockPos && currentLine.at(offset) == QLatin1Char('*'))
                    ++offset;
                const QChar ch = leadingAsterisks ? QLatin1Char('*') : QLatin1Char(' ');
                newLine.append(QString(offset - start, ch));
            }
        }
        for (; offset < blockPos && currentLine.at(offset) == ' '; ++offset)
            newLine.append(QLatin1Char(' '));
        cursor.insertText(newLine);
        return true;
    }

    return false;
}

static bool trySplitComment(TextEditorWidget *editorWidget,
                            const CPlusPlus::Snapshot &snapshot)
{
    const CommentsSettings::Data settings =
            ProjectExplorer::commentsSettingsForFile(editorWidget->textDocument()->filePath());

    if (!settings.enableDoxygen && !settings.leadingAsterisks)
        return false;

    if (editorWidget->multiTextCursor().hasMultipleCursors())
        return false;

    QTextCursor cursor = editorWidget->textCursor();
    if (!CPlusPlus::MatchingText::isInCommentHelper(cursor))
        return false;

    // We are interested on two particular cases:
    //   1) The cursor is right after a /**, /*!, /// or ///! and the user pressed enter.
    //      If Doxygen is enabled we need to generate an entire comment block.
    //   2) The cursor is already in the middle of a multi-line comment and the user pressed
    //      enter. If leading asterisk(s) is set we need to write a comment continuation
    //      with those.

    if (settings.enableDoxygen && cursor.positionInBlock() >= 3) {
        const int pos = cursor.position();
        if (isStartOfDoxygenComment(cursor)) {
            QTextDocument *textDocument = editorWidget->document();
            DoxygenGenerator::DocumentationStyle style = doxygenStyle(cursor, textDocument);

            // Check if we're already in a CppStyle Doxygen comment => continuation
            // Needs special handling since CppStyle does not have start and end markers
            if ((style == DoxygenGenerator::CppStyleA || style == DoxygenGenerator::CppStyleB)
                    && isCppStyleContinuation(cursor)) {
                return handleDoxygenCppStyleContinuation(cursor);
            }

            DoxygenGenerator doxygen;
            doxygen.setStyle(style);
            doxygen.setSettings(settings);

            // Move until we reach any possibly meaningful content.
            while (textDocument->characterAt(cursor.position()).isSpace()
                   && cursor.movePosition(QTextCursor::NextCharacter)) {
            }

            if (!cursor.atEnd()) {
                const QString &comment = doxygen.generate(cursor,
                                                          snapshot,
                                                          editorWidget->textDocument()->filePath());
                if (!comment.isEmpty()) {
                    cursor.beginEditBlock();
                    cursor.setPosition(pos);
                    cursor.insertText(comment);
                    cursor.setPosition(pos - 3, QTextCursor::KeepAnchor);
                    editorWidget->textDocument()->autoIndent(cursor);
                    cursor.endEditBlock();
                    return true;
                }
                cursor.setPosition(pos);
            }
        }
    } // right after first doxygen comment

    return handleDoxygenContinuation(cursor,
                                     editorWidget,
                                     settings.enableDoxygen,
                                     settings.leadingAsterisks);
}

} // anonymous namespace

class CppEditorWidgetPrivate
{
public:
    CppEditorWidgetPrivate(CppEditorWidget *q);

    bool shouldOfferOutline() const { return !CppModelManager::usesClangd(m_cppEditorDocument); }

public:
    CppEditorDocument *m_cppEditorDocument;
    CppEditorOutline *m_cppEditorOutline = nullptr;

    QTimer m_updateFunctionDeclDefLinkTimer;
    SemanticInfo m_lastSemanticInfo;

    FunctionDeclDefLinkFinder *m_declDefLinkFinder;
    std::shared_ptr<FunctionDeclDefLink> m_declDefLink;

    QAction *m_parseContextAction = nullptr;
    ParseContextWidget *m_parseContextWidget = nullptr;
    QToolButton *m_preprocessorButton = nullptr;

    CppLocalRenaming m_localRenaming;
    CppFunctionParamRenamingHandler m_paramRenamingHandler;
    CppUseSelectionsUpdater m_useSelectionsUpdater;
    CppSelectionChanger m_cppSelectionChanger;
    bool inTestMode = false;
};

CppEditorWidgetPrivate::CppEditorWidgetPrivate(CppEditorWidget *q)
    : m_cppEditorDocument(qobject_cast<CppEditorDocument *>(q->textDocument()))
    , m_declDefLinkFinder(new FunctionDeclDefLinkFinder(q))
    , m_localRenaming(q)
    , m_paramRenamingHandler(*q, m_localRenaming)
    , m_useSelectionsUpdater(q)
    , m_cppSelectionChanger()
{}
} // namespace Internal

using namespace Internal;

CppEditorWidget::CppEditorWidget()
    : d(new CppEditorWidgetPrivate(this))
{
    qRegisterMetaType<SemanticInfo>("SemanticInfo");
}

const QList<CppEditorWidget *> CppEditorWidget::editorWidgetsForDocument(
    TextEditor::TextDocument *doc)
{
    const QList<BaseTextEditor *> editors = BaseTextEditor::textEditorsForDocument(doc);
    const QList<TextEditorWidget *> editorWidgets
        = Utils::transform(editors, &BaseTextEditor::editorWidget);
    return Utils::qobject_container_cast<CppEditorWidget *>(editorWidgets);
}

void CppEditorWidget::finalizeInitialization()
{
    d->m_cppEditorDocument = qobject_cast<CppEditorDocument *>(textDocument());

    setLanguageSettingsId(Constants::CPP_SETTINGS_ID);

    // clang-format off
    // function combo box sorting
    d->m_cppEditorOutline = new CppEditorOutline(this);

    connect(d->m_cppEditorDocument, &CppEditorDocument::codeWarningsUpdated,
            this, &CppEditorWidget::onCodeWarningsUpdated);
    connect(d->m_cppEditorDocument, &CppEditorDocument::semanticInfoUpdated,
            this, [this](const SemanticInfo &info) { updateSemanticInfo(info); });

    connect(d->m_declDefLinkFinder, &FunctionDeclDefLinkFinder::foundLink,
            this, &CppEditorWidget::onFunctionDeclDefLinkFound);

    connect(&d->m_useSelectionsUpdater,
            &CppUseSelectionsUpdater::selectionsForVariableUnderCursorUpdated,
            &d->m_localRenaming,
            &CppLocalRenaming::updateSelectionsForVariableUnderCursor);

    connect(&d->m_useSelectionsUpdater, &CppUseSelectionsUpdater::finished, this,
            [this] (SemanticInfo::LocalUseMap localUses, bool success) {
                if (success) {
                    d->m_lastSemanticInfo.localUsesUpdated = true;
                    d->m_lastSemanticInfo.localUses = localUses;
                }
    });

    connect(document(), &QTextDocument::contentsChange,
            &d->m_localRenaming, &CppLocalRenaming::onContentsChangeOfEditorWidgetDocument);
    connect(&d->m_localRenaming, &CppLocalRenaming::finished, this, [this] {
        cppEditorDocument()->recalculateSemanticInfoDetached();
    });
    connect(&d->m_localRenaming, &CppLocalRenaming::processKeyPressNormally,
            this, &CppEditorWidget::processKeyNormally);
    connect(this, &PlainTextEdit::cursorPositionChanged, this, [this] {
        if (d->m_cppEditorOutline)
            d->m_cppEditorOutline->updateIndex();
    });

    connect(cppEditorDocument(), &CppEditorDocument::preprocessorSettingsChanged, this,
            [this](bool customSettings) {
        updateWidgetHighlighting(d->m_preprocessorButton, customSettings);
    });

    // set up function declaration - definition link
    d->m_updateFunctionDeclDefLinkTimer.setSingleShot(true);
    d->m_updateFunctionDeclDefLinkTimer.setInterval(UPDATE_FUNCTION_DECL_DEF_LINK_INTERVAL);
    connect(&d->m_updateFunctionDeclDefLinkTimer, &QTimer::timeout,
            this, &CppEditorWidget::updateFunctionDeclDefLinkNow);
    connect(this, &PlainTextEdit::cursorPositionChanged, this, &CppEditorWidget::updateFunctionDeclDefLink);
    connect(this, &PlainTextEdit::textChanged, this, &CppEditorWidget::updateFunctionDeclDefLink);

    // set up the use highlighitng
    connect(this, &CppEditorWidget::cursorPositionChanged, this, [this] {
        d->m_useSelectionsUpdater.scheduleUpdate();

        // Notify selection expander about the changed cursor.
        d->m_cppSelectionChanger.onCursorPositionChanged(textCursor());
    });

    // Toolbar: Parse context
    ParseContextModel &parseContextModel = cppEditorDocument()->parseContextModel();
    d->m_parseContextWidget = new ParseContextWidget(parseContextModel, this);
    d->m_parseContextAction = insertExtraToolBarWidget(TextEditorWidget::Left,
                                                       d->m_parseContextWidget);
    d->m_parseContextAction->setVisible(false);
    connect(&parseContextModel, &ParseContextModel::updated,
            this, [this](bool areMultipleAvailable) {
        d->m_parseContextAction->setVisible(areMultipleAvailable);
    });

    // Toolbar: Outline/Overview combo box
    setToolbarOutline(d->m_cppEditorOutline->widget());

    // clang-format on
    // Toolbar: '#' Button
    d->m_preprocessorButton = new QToolButton(this);
    d->m_preprocessorButton->setText(QLatin1String("#"));
    Command *cmd = ActionManager::command(Constants::OPEN_PREPROCESSOR_DIALOG);
    connect(cmd, &Command::keySequenceChanged,
            this, &CppEditorWidget::updatePreprocessorButtonTooltip);
    updatePreprocessorButtonTooltip();
    connect(d->m_preprocessorButton, &QAbstractButton::clicked,
            this, &CppEditorWidget::showPreProcessorWidget);
    insertExtraToolBarWidget(TextEditorWidget::Left, d->m_preprocessorButton);

    connect(this, &TextEditor::TextEditorWidget::toolbarOutlineChanged,
            this, &CppEditorWidget::handleOutlineChanged);
}

void CppEditorWidget::finalizeInitializationAfterDuplication(TextEditorWidget *other)
{
    QTC_ASSERT(other, return);
    auto cppEditorWidget = qobject_cast<CppEditorWidget *>(other);
    QTC_ASSERT(cppEditorWidget, return);

    if (cppEditorWidget->isSemanticInfoValidExceptLocalUses())
        updateSemanticInfo(cppEditorWidget->semanticInfo());
    const Id selectionKind = CodeWarningsSelection;
    setExtraSelections(selectionKind, cppEditorWidget->extraSelections(selectionKind));

    if (isWidgetHighlighted(cppEditorWidget->d->m_preprocessorButton))
        updateWidgetHighlighting(d->m_preprocessorButton, true);

    d->m_parseContextWidget->syncToModel();
    d->m_parseContextAction->setVisible(
                d->m_cppEditorDocument->parseContextModel().areMultipleAvailable());
}

void CppEditorWidget::setProposals(const TextEditor::IAssistProposal *immediateProposal,
                                   const TextEditor::IAssistProposal *finalProposal)
{
    QTC_ASSERT(isInTestMode(), return);
#ifdef WITH_TESTS
    emit proposalsReady(immediateProposal, finalProposal);
#else
    Q_UNUSED(immediateProposal)
    Q_UNUSED(finalProposal)
#endif
}

CppEditorWidget::~CppEditorWidget() = default;

namespace {

// A line of code up to a position, lexed in the state the highlighter left the line in
// front of it in. The tokens leave out the comments.
struct CodeLine
{
    bool startsWith(Kind kind) const { return !tokens.isEmpty() && tokens.first().is(kind); }
    bool endsWith(Kind kind) const { return !tokens.isEmpty() && tokens.last().is(kind); }

    QStringView spell(const Token &token) const
    {
        return QStringView(text).mid(token.utf16charsBegin(), token.utf16chars());
    }

    // The text up to the end of the code, without a comment that follows it.
    QString code() const
    {
        return tokens.isEmpty() ? QString() : text.left(tokens.last().utf16charsEnd()).trimmed();
    }

    QTextBlock block;
    QString text;
    Tokens tokens;
};

} // namespace

static CodeLine codeLine(const QTextBlock &block, int length = -1)
{
    SimpleLexer tokenize;
    tokenize.setLanguageFeatures(LanguageFeatures::defaultFeatures());
    tokenize.setSkipComments(true);
    tokenize.setExpectedRawStringSuffix(
        TextBlockUserData::expectedRawStringSuffix(block.previous()));
    CodeLine line{block, length < 0 ? block.text() : block.text().left(length), {}};
    line.tokens = tokenize(line.text, BackwardsScanner::previousBlockState(block));
    return line;
}

namespace {

// How many more parentheses, brackets and template argument lists some code opens than it
// closes.
struct Nesting
{
    bool opens() const { return parentheses > 0 || brackets > 0 || angles > 0; }
    // A '>' that closes a template argument list cannot be told from one that compares.
    bool closes() const { return parentheses < 0 || brackets < 0; }

    Nesting &operator+=(const Nesting &other)
    {
        parentheses += other.parentheses;
        brackets += other.brackets;
        angles += other.angles;
        return *this;
    }

    int parentheses = 0;
    int brackets = 0;
    int angles = 0;
};

} // namespace

// Only a '<' after "template", or right between a name and an argument, opens a template
// argument list, and only a '>' right after an argument closes one; "a < b" and "a > b"
// compare.
static Nesting nesting(const CodeLine &line)
{
    Nesting result;
    const Tokens &tokens = line.tokens;
    for (int i = 0; i < tokens.size(); ++i) {
        const Token &token = tokens.at(i);
        const bool afterPrevious = i > 0
                                   && tokens.at(i - 1).utf16charsEnd() == token.utf16charsBegin();
        const bool beforeNext = i + 1 == tokens.size()
                                || tokens.at(i + 1).utf16charsBegin() == token.utf16charsEnd();
        switch (token.kind()) {
        case T_LPAREN:
            ++result.parentheses;
            break;
        case T_RPAREN:
            --result.parentheses;
            break;
        case T_LBRACKET:
            ++result.brackets;
            break;
        case T_RBRACKET:
            --result.brackets;
            break;
        case T_LESS:
            if ((i > 0 && tokens.at(i - 1).is(T_TEMPLATE))
                || (afterPrevious && tokens.at(i - 1).is(T_IDENTIFIER) && beforeNext)) {
                ++result.angles;
            }
            break;
        case T_GREATER:
            if (afterPrevious)
                --result.angles;
            break;
        case T_GREATER_GREATER:
            if (afterPrevious)
                result.angles -= 2;
            break;
        default:
            break;
        }
    }
    return result;
}

// "} else {" closes one scope and opens the next.
static QString withoutClosingBraces(QString text)
{
    text = text.trimmed();
    while (text.startsWith('}'))
        text.remove(0, 1);
    return text.trimmed();
}

// The code in front of the brace at "position" in "block", or the previous line of code
// if the brace starts its line, joined with the lines of a parameter list, condition,
// capture list or template argument list that continues over several lines.
static QString headerLabel(const QTextBlock &block, int position)
{
    static const int maximumHeaderLines = 8;

    CodeLine line = codeLine(block, position);
    while (line.startsWith(T_RBRACE))
        line.tokens.removeFirst();
    while (line.tokens.isEmpty() && line.block.previous().isValid())
        line = codeLine(line.block.previous());
    QString label = withoutClosingBraces(line.text);
    Nesting labelNesting = nesting(line);

    // Lines that end with a comma are only part of the header once they turn out to be in
    // an open call, capture or template argument list, as siblings in a list end with one,
    // too. The label itself does not tell, as it can open a call of its own.
    QString pending;
    Nesting pendingNesting;
    QTextBlock previousBlock = line.block;
    for (int i = 0; i < maximumHeaderLines && (previousBlock = previousBlock.previous()).isValid();
         ++i) {
        const CodeLine previous = codeLine(previousBlock);
        const QString lines = previous.code() + ' ' + pending;
        Nesting linesNesting = nesting(previous);
        linesNesting += pendingNesting;
        if (labelNesting.closes() || previous.endsWith(T_LPAREN)
            || previous.endsWith(T_AMPER_AMPER) || previous.endsWith(T_PIPE_PIPE)
            || (previous.endsWith(T_COMMA) && linesNesting.opens())) {
            label = lines + label;
            labelNesting += linesNesting;
            pending.clear();
            pendingNesting = {};
        } else if (previous.endsWith(T_COMMA)) {
            pending = lines;
            pendingNesting = linesNesting;
        } else {
            break;
        }
    }
    return withoutClosingBraces(label).simplified();
}

// Whether "line" ends with a function body, as in "Foo() : a(0) {}", rather than with
// a member or base initialized with braces, as in ": a{1}" or ": Base<T>{}". A body
// follows a parameter list, an initializer, or a trailing specifier.
static bool endsWithBody(const CodeLine &line)
{
    if (!line.endsWith(T_RBRACE))
        return false;
    int depth = 0;
    for (qsizetype i = line.tokens.size() - 1; i >= 0; --i) {
        const Token &token = line.tokens.at(i);
        if (token.is(T_RBRACE)) {
            ++depth;
        } else if (token.is(T_LBRACE) && --depth == 0) {
            if (i == 0)
                return true;
            const Token &before = line.tokens.at(i - 1);
            switch (before.kind()) {
            case T_RPAREN:
            case T_RBRACE:
            case T_CONST:
            case T_VOLATILE:
            case T_NOEXCEPT:
            case T_MUTABLE:
            case T_AMPER:
            case T_AMPER_AMPER:
                return true;
            case T_IDENTIFIER:
                return line.spell(before) == u"override" || line.spell(before) == u"final";
            default:
                return false;
            }
        }
    }
    return false;
}

// Whether "line" ends the code in front of a declaration.
static bool endsCodeInFront(const CodeLine &line)
{
    const auto opensParenthesis = [&line] {
        return Utils::anyOf(line.tokens, [](const Token &token) { return token.is(T_LPAREN); });
    };
    return line.tokens.isEmpty() || line.startsWith(T_POUND) || line.startsWith(T_RBRACE)
           || line.endsWith(T_SEMICOLON) || line.endsWith(T_LBRACE) || endsWithBody(line)
           || (line.endsWith(T_COLON) && !opensParenthesis());
}

// Whether "tokens" start with "class" or "struct", possibly after a template header and a
// requires clause.
static bool startsClassHead(const Tokens &tokens)
{
    qsizetype i = 0;
    if (!tokens.isEmpty() && tokens.first().is(T_TEMPLATE)) {
        int depth = 0;
        for (i = 1; i < tokens.size(); ++i) {
            if (tokens.at(i).is(T_LESS))
                ++depth;
            else if (tokens.at(i).is(T_GREATER))
                --depth;
            else if (tokens.at(i).is(T_GREATER_GREATER))
                depth -= 2;
            if (depth <= 0) {
                ++i;
                break;
            }
        }
    }
    if (i < tokens.size() && tokens.at(i).is(T_REQUIRES)) {
        while (i < tokens.size() && tokens.at(i).isNot(T_CLASS) && tokens.at(i).isNot(T_STRUCT))
            ++i;
    }
    return i < tokens.size() && (tokens.at(i).is(T_CLASS) || tokens.at(i).is(T_STRUCT));
}

namespace {

struct DeclarationColon
{
    enum Kind { None, InitializerList, BaseList };

    Kind kind = None;
    QTextBlock block;
    int position = 0;
    QString classHead; // The declaration up to the brace, for a base list.
};

} // namespace

// The colon that starts a constructor's member initializer list or a class's base list in
// front of the brace at "position" in "block".
static DeclarationColon declarationColon(QTextBlock block, int position)
{
    static const int maximumInitializerLines = 30;

    // The lines of the declaration, up to where the code in front of it ends.
    QList<CodeLine> lines;
    for (int i = 0; i < maximumInitializerLines && block.isValid(); ++i) {
        CodeLine line = codeLine(block, i == 0 ? position : -1);
        if (i > 0 && endsCodeInFront(line))
            break;
        lines.prepend(std::move(line));
        block = block.previous();
    }

    // The list follows the parameter list or class name, outside of any parentheses or
    // conditional.
    Tokens declaration;
    int depth = 0;
    bool conditional = false;
    for (const CodeLine &line : std::as_const(lines)) {
        for (const Token &token : line.tokens) {
            switch (token.kind()) {
            case T_LPAREN:
            case T_LBRACKET:
            case T_LBRACE:
                ++depth;
                break;
            case T_RPAREN:
            case T_RBRACKET:
            case T_RBRACE:
                --depth;
                break;
            case T_QUESTION:
                conditional = conditional || depth == 0;
                break;
            case T_COLON:
                if (depth != 0 || conditional || declaration.isEmpty())
                    break;
                if (declaration.last().is(T_RPAREN) || declaration.last().is(T_NOEXCEPT))
                    return {DeclarationColon::InitializerList, line.block,
                            token.utf16charsBegin(), {}};
                if (startsClassHead(declaration)) {
                    QString head;
                    for (const CodeLine &headLine : std::as_const(lines))
                        head += headLine.code() + ' ';
                    return {DeclarationColon::BaseList, line.block, token.utf16charsBegin(),
                            head.simplified()};
                }
                break;
            default:
                break;
            }
            declaration.append(token);
        }
    }
    return {};
}

// The declaration a constructor's member initializer list belongs to, not the list itself,
// and all of a class head whose base list continues over several lines.
QString CppEditorWidget::scopeLabelText(const QTextBlock &openBlock, int openBracePosition) const
{
    const DeclarationColon colon = declarationColon(openBlock, openBracePosition);
    switch (colon.kind) {
    case DeclarationColon::InitializerList:
        return headerLabel(colon.block, colon.position);
    case DeclarationColon::BaseList:
        return colon.classHead;
    case DeclarationColon::None:
        break;
    }
    return headerLabel(openBlock, openBracePosition);
}

enum class Directive { None, If, Endif };

// "rest" is what follows the directive's name.
static Directive conditionalDirective(const QString &text, QString *rest)
{
    qsizetype i = 0;
    const auto skipSpaces = [&] {
        while (i < text.size() && text.at(i).isSpace())
            ++i;
    };
    skipSpaces();
    if (i >= text.size() || text.at(i) != '#')
        return Directive::None;
    ++i;
    skipSpaces();
    const qsizetype nameStart = i;
    while (i < text.size() && text.at(i).isLetter())
        ++i;
    const QStringView name = QStringView(text).sliced(nameStart, i - nameStart);
    if (name == u"if" || name == u"ifdef" || name == u"ifndef")
        return Directive::If;
    if (name == u"endif") {
        *rest = text.mid(i).trimmed();
        return Directive::Endif;
    }
    return Directive::None;
}

// The highlighter stores the state of the lexer, CPlusPlus::Lexer::State, as the state of a
// block: the kind of the token that continues into the next line, 0 outside of comments and
// strings, in the low 7 bits, and a flag for block comments in the high bit.
static bool startsInCode(const QTextBlock &block)
{
    static const int tokenKindMask = 0x7f;

    const QTextBlock previous = block.previous();
    return !previous.isValid() || previous.userState() == -1
           || (previous.userState() & tokenKindMask) == 0;
}

// An "#endif" that has a comment already says what it ends.
QTextBlock CppEditorWidget::conditionalStartBlock(const QTextBlock &endBlock) const
{
    static const int maximumConditionalLines = 10000;

    QString rest;
    if (!startsInCode(endBlock) || conditionalDirective(endBlock.text(), &rest) != Directive::Endif
        || !rest.isEmpty()) {
        return {};
    }

    int depth = 1;
    QTextBlock block = endBlock;
    for (int i = 0; i < maximumConditionalLines && (block = block.previous()).isValid(); ++i) {
        if (!startsInCode(block))
            continue;
        switch (conditionalDirective(block.text(), &rest)) {
        case Directive::If:
            if (--depth == 0)
                return block;
            break;
        case Directive::Endif:
            ++depth;
            break;
        case Directive::None:
            break;
        }
    }
    return {};
}

CppEditorDocument *CppEditorWidget::cppEditorDocument() const
{
    return d->m_cppEditorDocument;
}

void CppEditorWidget::paste()
{
    if (d->m_localRenaming.handlePaste())
        return;

    TextEditorWidget::paste();
}

void CppEditorWidget::cut()
{
    if (d->m_localRenaming.handleCut())
        return;

    TextEditorWidget::cut();
}

void CppEditorWidget::selectAll()
{
    if (d->m_localRenaming.handleSelectAll())
        return;

    TextEditorWidget::selectAll();
}

void CppEditorWidget::onCodeWarningsUpdated(unsigned revision,
                                            const QList<QTextEdit::ExtraSelection> selections,
                                            const RefactorMarkers &refactorMarkers)
{
    if (revision != documentRevision())
        return;

    setExtraSelections(TextEditorWidget::CodeWarningsSelection,
                       unselectLeadingWhitespace(selections));
    setRefactorMarkers(refactorMarkers, Constants::CPP_CLANG_FIXIT_AVAILABLE_MARKER_ID);
}

void CppEditorWidget::findUsages()
{
    findUsages(textCursor());
}

void CppEditorWidget::findUsages(QTextCursor cursor)
{
    // 'this' in cursorInEditor is never used (and must never be used) asynchronously.
    const CursorInEditor cursorInEditor{cursor, textDocument()->filePath(), this, textDocument()};
    QPointer<CppEditorWidget> cppEditorWidget = this;
    CppModelManager::findUsages(cursorInEditor);
}

void CppEditorWidget::renameUsages(const QString &replacement, QTextCursor cursor)
{
    if (cursor.isNull())
        cursor = textCursor();

    // First check if the symbol to be renamed comes from a generated file.
    LinkHandler continuation = [this, cursor, replacement, self = QPointer(this)](const Link &link) {
        if (!self)
            return;
        showRenameWarningIfFileIsGenerated(link.targetFilePath);
        const CursorInEditor cursorInEditor{cursor, textDocument()->filePath(), this, textDocument()};
        CppModelManager::globalRename(cursorInEditor, replacement);
    };
    NonInteractiveFollowSymbolMarker niMarker;
    CppModelManager::followSymbol(CursorInEditor{cursor,
                                                 textDocument()->filePath(),
                                                 this,
                                                 textDocument()},
                                  continuation,
                                  false,
                                  false,
                                  FollowSymbolMode::Exact);
}

void CppEditorWidget::renameUsages(const Utils::FilePath &filePath, const QString &replacement,
                                   QTextCursor cursor, const std::function<void ()> &callback)
{
    if (cursor.isNull())
        cursor = textCursor();
    CursorInEditor cursorInEditor{cursor, filePath, this, textDocument()};
    QPointer<CppEditorWidget> cppEditorWidget = this;
    CppModelManager::globalRename(cursorInEditor, replacement, callback);
}

bool CppEditorWidget::selectBlockUp()
{
    if (!behaviorSettings().m_smartSelectionChanging)
        return TextEditorWidget::selectBlockUp();

    QTextCursor cursor = textCursor();
    d->m_cppSelectionChanger.startChangeSelection();
    const bool changed = d->m_cppSelectionChanger
                             .changeSelection(CppSelectionChanger::ExpandSelection,
                                              cursor,
                                              d->m_lastSemanticInfo.doc);
    if (changed)
        setTextCursor(cursor);
    d->m_cppSelectionChanger.stopChangeSelection();

    return changed;
}

bool CppEditorWidget::selectBlockDown()
{
    if (!behaviorSettings().m_smartSelectionChanging)
        return TextEditorWidget::selectBlockDown();

    QTextCursor cursor = textCursor();
    d->m_cppSelectionChanger.startChangeSelection();
    const bool changed = d->m_cppSelectionChanger
                             .changeSelection(CppSelectionChanger::ShrinkSelection,
                                              cursor,
                                              d->m_lastSemanticInfo.doc);
    if (changed)
        setTextCursor(cursor);
    d->m_cppSelectionChanger.stopChangeSelection();

    return changed;
}

void CppEditorWidget::updateWidgetHighlighting(QWidget *widget, bool highlight)
{
    if (!widget)
        return;

    widget->setProperty(StyleHelper::C_HIGHLIGHT_WIDGET, highlight);
    widget->update();
}

bool CppEditorWidget::isWidgetHighlighted(QWidget *widget)
{
    return widget ? widget->property(StyleHelper::C_HIGHLIGHT_WIDGET).toBool() : false;
}

namespace {

QList<ProjectPart::ConstPtr> fetchProjectParts(const Utils::FilePath &filePath)
{
    QList<ProjectPart::ConstPtr> projectParts = CppModelManager::projectPart(filePath);

    if (projectParts.isEmpty())
        projectParts = CppModelManager::projectPartFromDependencies(filePath);
    if (projectParts.isEmpty())
        projectParts.append(CppModelManager::fallbackProjectPart());

    return projectParts;
}

const ProjectPart *findProjectPartForCurrentProject(
        const QList<ProjectPart::ConstPtr> &projectParts,
        ProjectExplorer::Project *currentProject)
{
    const auto found = std::find_if(projectParts.cbegin(),
                              projectParts.cend(),
                              [&](const ProjectPart::ConstPtr &projectPart) {
                                  return projectPart->belongsToProject(currentProject);
                              });

    if (found != projectParts.cend())
        return (*found).data();

    return nullptr;
}

} // namespace

const ProjectPart *CppEditorWidget::projectPart() const
{
    if (!CppModelManager::instance())
        return nullptr;

    auto projectParts = fetchProjectParts(textDocument()->filePath());

    return findProjectPartForCurrentProject(projectParts,
                                            ProjectExplorer::ProjectTree::currentProject());
}

void CppEditorWidget::handleOutlineChanged(const QWidget *newOutline)
{
    if (d->m_cppEditorOutline && newOutline != d->m_cppEditorOutline->widget()) {
        delete d->m_cppEditorOutline;
        d->m_cppEditorOutline = nullptr;
    }
    if (newOutline == nullptr) {
        if (!d->m_cppEditorOutline)
            d->m_cppEditorOutline = new CppEditorOutline(this);
        d->m_cppEditorOutline->updateIndex();
        setToolbarOutline(d->m_cppEditorOutline->widget());
    }
}

void CppEditorWidget::showRenameWarningIfFileIsGenerated(const Utils::FilePath &filePath)
{
    if (filePath.isEmpty())
        return;
    for (const Project * const project : ProjectManager::projects()) {
        const Node * const node = project->nodeForFilePath(filePath);
        if (!node)
            continue;
        if (!node->isGenerated())
            return;
        ExtraCompiler *ec = nullptr;
        QString warning = CppEditor::Tr::tr(
                    "You are trying to rename a symbol declared in the generated file \"%1\".\n"
                    "This is normally not a good idea, as the file will likely get "
                    "overwritten during the build process.")
                .arg(filePath.toUserOutput());
        if (const BuildSystem * const bs = project->activeBuildSystem())
            ec = bs->extraCompilerForTarget(filePath);
        if (ec) {
            warning.append('\n').append(CppEditor::Tr::tr(
                                            "Do you want to edit \"%1\" instead?")
                                        .arg(ec->source().toUserOutput()));
        }
        static const Id infoId("cppeditor.renameWarning");
        InfoBarEntry info(infoId, warning);
        info.setTitle(Tr::tr("Renamed Symbol Will Be Overwritten"));
        info.setInfoType(InfoLabelType::Warning);
        if (ec) {
            info.addCustomButton(
                CppEditor::Tr::tr("Open \"%1\"").arg(ec->source().fileName()),
                [source = ec->source()] { EditorManager::openEditor(source); },
                {},
                InfoBarEntry::ButtonAction::Hide);
        }
        ICore::popupInfoBar()->addInfo(info);
        return;
    }
}

namespace {

using Utils::Text::selectAt;

QTextCharFormat occurrencesTextCharFormat()
{
    return globalFontSettings().data().toTextCharFormat(TextEditor::C_OCCURRENCES);
}

QList<QTextEdit::ExtraSelection> sourceLocationsToExtraSelections(
    const Links &sourceLocations,
    uint selectionLength,
    CppEditorWidget *cppEditorWidget)
{
    const auto textCharFormat = occurrencesTextCharFormat();

    QList<QTextEdit::ExtraSelection> selections;
    selections.reserve(int(sourceLocations.size()));

    auto sourceLocationToExtraSelection = [&](const Link &sourceLocation) {
        QTextEdit::ExtraSelection selection;

        selection.cursor = selectAt(cppEditorWidget->textCursor(),
                                    sourceLocation.target.line,
                                    sourceLocation.target.column,
                                    selectionLength);
        selection.format = textCharFormat;

        return selection;
    };

    std::transform(sourceLocations.begin(),
                   sourceLocations.end(),
                   std::back_inserter(selections),
                   sourceLocationToExtraSelection);

    return selections;
};

}

void CppEditorWidget::renameSymbolUnderCursor()
{
    const ProjectPart *projPart = projectPart();
    if (!projPart)
        return;

    if (d->m_localRenaming.isActive()
            && d->m_localRenaming.isSameSelection(textCursor().position())) {
        return;
    }
    d->m_useSelectionsUpdater.abortSchedule();

    QPointer<CppEditorWidget> cppEditorWidget = this;

    auto renameSymbols = [this, cppEditorWidget](const QString &symbolName, const Links &links,
                                                 int revision) {
        if (cppEditorWidget) {
            viewport()->setCursor(Qt::IBeamCursor);

            if (revision != document()->revision())
                return;
            if (!links.isEmpty()) {
                QList<QTextEdit::ExtraSelection> selections
                    = sourceLocationsToExtraSelections(links,
                                                       static_cast<uint>(symbolName.size()),
                                                       cppEditorWidget);
                setExtraSelections(TextEditor::TextEditorWidget::CodeSemanticsSelection, selections);
                d->m_localRenaming.stop();
                d->m_localRenaming.updateSelectionsForVariableUnderCursor(selections);
            }
            if (!d->m_localRenaming.start())
                cppEditorWidget->renameUsages();
        }
    };

    viewport()->setCursor(Qt::BusyCursor);
    CppModelManager::startLocalRenaming(CursorInEditor{textCursor(),
                                                       textDocument()->filePath(),
                                                       this, textDocument()},
                                        projPart,
                                        std::move(renameSymbols));
}

void CppEditorWidget::inInlineRename(bool *active)
{
    *active = d->m_localRenaming.isActive();
}

void CppEditorWidget::updatePreprocessorButtonTooltip()
{
    if (!d->m_preprocessorButton)
        return;

    Command *cmd = ActionManager::command(Constants::OPEN_PREPROCESSOR_DIALOG);
    QTC_ASSERT(cmd, return );
    d->m_preprocessorButton->setToolTip(cmd->action()->toolTip());
}

void CppEditorWidget::switchDeclarationDefinition(bool inNextSplit)
{
    if (!CppModelManager::instance())
        return;

    const CursorInEditor cursor(textCursor(), textDocument()->filePath(), this, textDocument());
    auto callback = [self = QPointer(this),
            split = inNextSplit != alwaysOpenLinksInNextSplit()](const Link &link) {
        if (self && link.hasValidTarget())
            self->openLink(link, split);
    };
    CppModelManager::switchDeclDef(cursor, std::move(callback));
}

void CppEditorWidget::goToParentImpl(bool inNextSplit)
{
    if (!CppModelManager::instance())
        return;

    const CursorInEditor cursor(textCursor(), textDocument()->filePath(), this, textDocument());
    auto callback = [self = QPointer(this),
                     split = inNextSplit != alwaysOpenLinksInNextSplit()](const Link &link) {
        if (self && link.hasValidTarget())
            self->openLink(link, split);
    };
    CppModelManager::followFunctionToParentImpl(cursor, callback);
}

bool CppEditorWidget::followUrl(const QTextCursor &cursor,
                                   const Utils::LinkHandler &processLinkCallback)
{
    if (!isSemanticInfoValidExceptLocalUses())
        return false;

    const Project * const project = ProjectTree::currentProject();
    if (!project || !project->rootProjectNode())
        return false;

    const QList<AST *> astPath = ASTPath(d->m_lastSemanticInfo.doc)(cursor);
    if (astPath.isEmpty())
        return false;
    const StringLiteralAST * const literalAst = astPath.last()->asStringLiteral();
    if (!literalAst)
        return false;
    const StringLiteral * const literal = d->m_lastSemanticInfo.doc->translationUnit()
            ->stringLiteral(literalAst->literal_token);
    if (!literal)
        return false;
    const QString theString = QString::fromUtf8(literal->chars(), literal->size());

    if (theString.startsWith("https:/") || theString.startsWith("http:/")) {
        Utils::Link link = FilePath::fromPathPart(theString);
        link.linkTextStart = d->m_lastSemanticInfo.doc->translationUnit()->getTokenPositionInDocument(literalAst->literal_token, document());
        link.linkTextEnd = d->m_lastSemanticInfo.doc->translationUnit()->getTokenEndPositionInDocument(literalAst->literal_token, document());
        processLinkCallback(link);
        return true;
    }

    if (!theString.startsWith("qrc:/") && !theString.startsWith(":/"))
        return false;

    const Node * const nodeForPath = project->rootProjectNode()->findNode(
                [qrcPath = theString.mid(theString.indexOf(':') + 1)](Node *n) {
        if (!n->asFileNode())
            return false;
        const auto qrcNode = dynamic_cast<ResourceFileNode *>(n);
        return qrcNode && qrcNode->qrcPath() == qrcPath;
    });
    if (!nodeForPath)
        return false;

    Link link(nodeForPath->filePath());
    link.linkTextStart = d->m_lastSemanticInfo.doc->translationUnit()->getTokenPositionInDocument(literalAst->literal_token, document());
    link.linkTextEnd = d->m_lastSemanticInfo.doc->translationUnit()->getTokenEndPositionInDocument(literalAst->literal_token, document());
    processLinkCallback(link);
    return true;
}

void CppEditorWidget::findLinkAt(const QTextCursor &cursor,
                                 const LinkHandler &processLinkCallback,
                                 bool resolveTarget,
                                 bool inNextSplit)
{
    if (!CppModelManager::instance())
        return processLinkCallback(Utils::Link());

    if (followUrl(cursor, processLinkCallback))
        return;

    const Utils::FilePath &filePath = textDocument()->filePath();

    // Let following a "leaf" C++ symbol take us to the designer, if we are in a generated
    // UI header.
    QTextCursor c(cursor);
    c.select(QTextCursor::WordUnderCursor);
    LinkHandler callbackWrapper = [start = c.selectionStart(), end = c.selectionEnd(),
            doc = QPointer(cursor.document()), callback = processLinkCallback,
            filePath](const Link &link) {
        const int linkPos = doc ? link.target.toPositionInDocument(doc) : -1;
        if (link.targetFilePath == filePath && linkPos >= start && linkPos < end) {
            const QString fileName = filePath.fileName();
            if (fileName.startsWith("ui_") && fileName.endsWith(".h")) {
                const QString uiFileName = fileName.mid(3, fileName.size() - 4) + "ui";
                for (const Project * const project : ProjectManager::projects()) {
                    const auto nodeMatcher = [uiFileName](Node *n) {
                        return n->filePath().fileName() == uiFileName;
                    };
                    ProjectNode *rootNode = project->rootProjectNode();
                    if (!rootNode)
                        continue;
                    if (const Node * const uiNode = rootNode->findNode(nodeMatcher)) {
                        EditorManager::openEditor(uiNode->filePath());
                        return;
                    }
                }
            }
        }
        callback(link);
    };
    CppModelManager::followSymbol(CursorInEditor{cursor, filePath, this, textDocument()},
                                  callbackWrapper,
                                  resolveTarget,
                                  inNextSplit,
                                  FollowSymbolMode::Fuzzy);
}

void CppEditorWidget::findTypeAt(const QTextCursor &cursor,
                                 const Utils::LinkHandler &processLinkCallback,
                                 bool /*resolveTarget*/,
                                 bool inNextSplit)
{
    if (!CppModelManager::instance())
        return;

    const CursorInEditor cursorInEditor(cursor, textDocument()->filePath(), this, textDocument());
    CppModelManager::followSymbolToType(cursorInEditor, processLinkCallback, inNextSplit);
}

unsigned CppEditorWidget::documentRevision() const
{
    return document()->revision();
}

bool CppEditorWidget::isSemanticInfoValidExceptLocalUses() const
{
    return d->m_lastSemanticInfo.doc && d->m_lastSemanticInfo.revision == documentRevision()
           && !d->m_lastSemanticInfo.snapshot.isEmpty();
}

bool CppEditorWidget::isSemanticInfoValid() const
{
    return isSemanticInfoValidExceptLocalUses() && d->m_lastSemanticInfo.localUsesUpdated;
}

bool CppEditorWidget::isRenaming() const
{
    return d->m_localRenaming.isActive();
}

SemanticInfo CppEditorWidget::semanticInfo() const
{
    return d->m_lastSemanticInfo;
}

bool CppEditorWidget::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        // handle escape manually if a rename is active
        if (static_cast<QKeyEvent *>(e)->key() == Qt::Key_Escape && d->m_localRenaming.isActive()) {
            e->accept();
            return true;
        }
        break;
    default:
        break;
    }

    return TextEditorWidget::event(e);
}

void CppEditorWidget::processKeyNormally(QKeyEvent *e)
{
    TextEditorWidget::keyPressEvent(e);
}

void CppEditorWidget::addRefactoringActions(QMenu *menu) const
{
    if (!menu)
        return;

    auto iface = createAssistInterface(QuickFix, ExplicitlyInvoked);
    IAssistProcessor * const processor
        = textDocument()->quickFixAssistProvider()->createProcessor(iface.get());
    IAssistProposal* const proposal(processor->start(std::move(iface)));
    const auto handleProposal = [menu = QPointer(menu), processor](IAssistProposal *proposal) {
        QScopedPointer<IAssistProposal> proposalHolder(proposal);
        QScopedPointer<IAssistProcessor> processorHolder(processor);
        if (!menu || !proposal)
            return;
        auto model = proposal->model().staticCast<GenericProposalModel>();
        for (int index = 0; index < model->size(); ++index) {
            const auto item = static_cast<AssistProposalItem *>(model->proposalItem(index));
            const QuickFixOperation::Ptr op = item->data().value<QuickFixOperation::Ptr>();
            const QAction *action = menu->addAction(op->description());
            QObject::connect(action, &QAction::triggered, menu, [op] { op->perform(); });
        }
    };

    if (proposal)
        handleProposal(proposal);
    else
        processor->setAsyncCompletionAvailableHandler(handleProposal);
}

class ProgressIndicatorMenuItem : public QWidgetAction
{
public:
    ProgressIndicatorMenuItem(QObject *parent) : QWidgetAction(parent) {}

protected:
    QWidget *createWidget(QWidget *parent = nullptr) override
    {
        return new Utils::ProgressIndicator(Utils::ProgressIndicatorSize::Small, parent);
    }
};

QMenu *CppEditorWidget::createRefactorMenu(QWidget *parent) const
{
    auto *menu = new QMenu(Tr::tr("&Refactor"), parent);
    connect(menu, &QMenu::aboutToShow, this, [this, menu] {
        menu->disconnect(this);

        // ### enable
        // updateSemanticInfo(m_semanticHighlighter->semanticInfo(currentSource()));

        if (!isSemanticInfoValidExceptLocalUses())
            return;

        d->m_useSelectionsUpdater.abortSchedule();

        const CppUseSelectionsUpdater::RunnerInfo runnerInfo = d->m_useSelectionsUpdater.update();
        switch (runnerInfo) {
        case CppUseSelectionsUpdater::RunnerInfo::AlreadyUpToDate:
            addRefactoringActions(menu);
            break;
        case CppUseSelectionsUpdater::RunnerInfo::Started: {
            // Update the refactor menu once we get the results.
            auto *progressIndicatorMenuItem = new ProgressIndicatorMenuItem(menu);
            menu->addAction(progressIndicatorMenuItem);

            connect(&d->m_useSelectionsUpdater, &CppUseSelectionsUpdater::finished, menu,
                    [this, menu, progressIndicatorMenuItem] (SemanticInfo::LocalUseMap, bool success) {
                QTC_CHECK(success);
                menu->removeAction(progressIndicatorMenuItem);
                addRefactoringActions(menu);
            });
            break;
        }
        case CppUseSelectionsUpdater::RunnerInfo::FailedToStart:
        case CppUseSelectionsUpdater::RunnerInfo::Invalid:
            QTC_CHECK(false && "Unexpected CppUseSelectionsUpdater runner result");
        }
        QMetaObject::invokeMethod(menu, [menu](){
            if (auto mainWin = ICore::mainWindow()) {
                menu->adjustSize();
                if (QTC_GUARD(menu->parentWidget())) {
                    QPoint p = menu->pos();
                    const int w = menu->width();
                    if (p.x() + w > mainWin->screen()->geometry().width()) {
                        p.setX(menu->parentWidget()->x() - w);
                        menu->move(p);
                    }
                }
            }
        }, Qt::QueuedConnection);
    });

    return menu;
}

static void appendCustomContextMenuActionsAndMenus(QMenu *menu, QMenu *refactorMenu)
{
    bool isRefactoringMenuAdded = false;
    const QMenu *contextMenu = ActionManager::actionContainer(Constants::M_CONTEXT)->menu();
    for (QAction *action : contextMenu->actions()) {
        if (action->objectName() == QLatin1String(Constants::M_REFACTORING_MENU_INSERTION_POINT)) {
            isRefactoringMenuAdded = true;
            menu->addMenu(refactorMenu);
        } else {
            menu->addAction(action);
        }
    }

    QTC_CHECK(isRefactoringMenuAdded);
}

void CppEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    const QPointer<QMenu> menu(new QMenu(this));

    appendCustomContextMenuActionsAndMenus(menu, createRefactorMenu(menu));
    appendStandardContextMenuActions(menu);
    appendSpellingActions(menu, e->pos());

    menu->exec(e->globalPos());
    if (menu)
        delete menu; // OK, menu was not already deleted by closed editor widget.
}

void CppEditorWidget::keyPressEvent(QKeyEvent *e)
{
    if (d->m_localRenaming.handleKeyPressEvent(e))
        return;

    if (handleStringSplitting(e))
        return;

    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        if (trySplitComment(this, semanticInfo().snapshot)) {
            e->accept();
            return;
        }
    }

    TextEditorWidget::keyPressEvent(e);
}

bool CppEditorWidget::handleStringSplitting(QKeyEvent *e) const
{
    if (!globalCompletionSettings().autoSplitStrings())
        return false;

    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        QTextCursor cursor = textCursor();

        const Kind stringKind = CPlusPlus::MatchingText::stringKindAtCursor(cursor);
        if (stringKind >= T_FIRST_STRING_LITERAL && stringKind < T_FIRST_RAW_STRING_LITERAL) {
            cursor.beginEditBlock();
            if (cursor.positionInBlock() > 0
                && cursor.block().text().at(cursor.positionInBlock() - 1) == QLatin1Char('\\')) {
                // Already escaped: simply go back to line, but do not indent.
                cursor.insertText(QLatin1String("\n"));
            } else if (e->modifiers() & Qt::ShiftModifier) {
                // With 'shift' modifier, escape the end of line character
                // and start at beginning of next line.
                cursor.insertText(QLatin1String("\\\n"));
            } else {
                // End the current string, and start a new one on the line, properly indented.
                cursor.insertText(QLatin1String("\"\n\""));
                textDocument()->autoIndent(cursor);
            }
            cursor.endEditBlock();
            e->accept();
            return true;
        }
    }

    return false;
}

void CppEditorWidget::updateSemanticInfo()
{
    updateSemanticInfo(d->m_cppEditorDocument->recalculateSemanticInfo(),
                       /*updateUseSelectionSynchronously=*/ true);
}

void CppEditorWidget::updateSemanticInfo(const SemanticInfo &semanticInfo,
                                         bool updateUseSelectionSynchronously)
{
    if (semanticInfo.revision < documentRevision())
        return;

    d->m_lastSemanticInfo = semanticInfo;

    const CppUseSelectionsUpdater::CallType type
        = updateUseSelectionSynchronously ? CppUseSelectionsUpdater::CallType::Synchronous
                                          : CppUseSelectionsUpdater::CallType::Asynchronous;
    d->m_useSelectionsUpdater.update(type);

    // schedule a check for a decl/def link
    updateFunctionDeclDefLink();
}

bool CppEditorWidget::isOldStyleSignalOrSlot() const
{
    return CppEditor::CppModelManager::instance()
               ->getSignalSlotType(textDocument()->filePath(), textCursor())
           == CppEditor::SignalSlotType::OldStyleSignal;
}

std::unique_ptr<AssistInterface> CppEditorWidget::createAssistInterface(AssistKind kind,
                                                                        AssistReason reason) const
{
    if (kind == Completion || kind == FunctionHint) {
        CppCompletionAssistProvider * const cap = kind == Completion
                ? qobject_cast<CppCompletionAssistProvider *>(cppEditorDocument()->completionAssistProvider())
                : qobject_cast<CppCompletionAssistProvider *>(cppEditorDocument()->functionHintAssistProvider());

        auto getFeatures = [this] {
            LanguageFeatures features = LanguageFeatures::defaultFeatures();
            if (Document::Ptr doc = d->m_lastSemanticInfo.doc)
                features = doc->languageFeatures();
            features.objCEnabled |= cppEditorDocument()->isObjCEnabled();
            return features;
        };

        if (cap)
            return cap->createAssistInterface(textDocument()->filePath(), this, getFeatures(), reason);

        if (isOldStyleSignalOrSlot()
            || isInCommentOrString(textCursor(), LanguageFeatures::defaultFeatures())) {
            return CppModelManager::completionAssistProvider()
                ->createAssistInterface(textDocument()->filePath(), this, getFeatures(), reason);
        }
    }
    if (kind == QuickFix && isSemanticInfoValid())
        return std::make_unique<CppQuickFixInterface>(const_cast<CppEditorWidget *>(this), reason);
    return TextEditorWidget::createAssistInterface(kind, reason);
}

std::shared_ptr<FunctionDeclDefLink> CppEditorWidget::declDefLink() const
{
    return d->m_declDefLink;
}

void CppEditorWidget::updateFunctionDeclDefLink()
{
    const int pos = textCursor().selectionStart();

    // if there's already a link, abort it if the cursor is outside or the name changed
    // (adding a prefix is an exception since the user might type a return type)
    if (d->m_declDefLink
        && (pos < d->m_declDefLink->linkSelection.selectionStart()
            || pos > d->m_declDefLink->linkSelection.selectionEnd()
            || !d->m_declDefLink->nameSelection.selectedText().trimmed().endsWith(
                   d->m_declDefLink->nameInitial))) {
        abortDeclDefLink();
        return;
    }

    // don't start a new scan if there's one active and the cursor is already in the scanned area
    const QTextCursor scannedSelection = d->m_declDefLinkFinder->scannedSelection();
    if (!scannedSelection.isNull() && scannedSelection.selectionStart() <= pos
        && scannedSelection.selectionEnd() >= pos) {
        return;
    }

    d->m_updateFunctionDeclDefLinkTimer.start();
}

void CppEditorWidget::updateFunctionDeclDefLinkNow()
{
    IEditor *editor = EditorManager::currentEditor();
    if (!editor || editor->widget() != this)
        return;

    const Snapshot semanticSnapshot = d->m_lastSemanticInfo.snapshot;
    const Document::Ptr semanticDoc = d->m_lastSemanticInfo.doc;

    if (d->m_declDefLink) {
        // update the change marker
        const Utils::ChangeSet changes = d->m_declDefLink->changes(semanticSnapshot);
        if (changes.isEmpty())
            d->m_declDefLink->hideMarker(this);
        else
            d->m_declDefLink->showMarker(this);
        return;
    }

    if (!isSemanticInfoValidExceptLocalUses())
        return;

    Snapshot snapshot = CppModelManager::snapshot();
    snapshot.insert(semanticDoc);

    d->m_declDefLinkFinder->startFindLinkAt(textCursor(), semanticDoc, snapshot);
}

void CppEditorWidget::onFunctionDeclDefLinkFound(std::shared_ptr<FunctionDeclDefLink> link)
{
    abortDeclDefLink();
    d->m_declDefLink = link;
    IDocument *targetDocument = DocumentModel::documentForFilePath(
        d->m_declDefLink->targetFile->filePath());
    if (textDocument() != targetDocument) {
        if (auto textDocument = qobject_cast<BaseTextDocument *>(targetDocument))
            connect(textDocument,
                    &IDocument::contentsChanged,
                    this,
                    &CppEditorWidget::abortDeclDefLink);
    }
}

void CppEditorWidget::applyDeclDefLinkChanges(bool jumpToMatch)
{
    if (!d->m_declDefLink)
        return;
    d->m_declDefLink->apply(this, jumpToMatch);
    abortDeclDefLink();
    updateFunctionDeclDefLink();
}

void CppEditorWidget::encourageApply()
{
    if (d->m_localRenaming.encourageApply())
        return;

    TextEditorWidget::encourageApply();
}

void CppEditorWidget::abortDeclDefLink()
{
    if (!d->m_declDefLink)
        return;

    IDocument *targetDocument = DocumentModel::documentForFilePath(
        d->m_declDefLink->targetFile->filePath());
    if (textDocument() != targetDocument) {
        if (auto textDocument = qobject_cast<BaseTextDocument *>(targetDocument))
            disconnect(textDocument,
                       &IDocument::contentsChanged,
                       this,
                       &CppEditorWidget::abortDeclDefLink);
    }

    d->m_declDefLink->hideMarker(this);
    d->m_declDefLink.reset();
}

void CppEditorWidget::showPreProcessorWidget()
{
    const FilePath filePath = textDocument()->filePath();

    CppPreProcessorDialog dialog(filePath, this);
    if (dialog.exec() == QDialog::Accepted) {
        const QByteArray extraDirectives = dialog.extraPreprocessorDirectives().toUtf8();
        cppEditorDocument()->setExtraPreprocessorDirectives(extraDirectives);
        cppEditorDocument()->scheduleProcessDocument();
    }
}

void CppEditorWidget::invokeTextEditorWidgetAssist(TextEditor::AssistKind assistKind,
                                                   TextEditor::IAssistProvider *provider)
{
    invokeAssist(assistKind, provider);
}

const QList<QTextEdit::ExtraSelection> CppEditorWidget::unselectLeadingWhitespace(
        const QList<QTextEdit::ExtraSelection> &selections)
{
    QList<QTextEdit::ExtraSelection> filtered;
    for (const QTextEdit::ExtraSelection &sel : selections) {
        QList<QTextEdit::ExtraSelection> splitSelections;
        int firstNonWhitespacePos = -1;
        int lastNonWhitespacePos = -1;
        bool split = false;
        const QTextBlock firstBlock = sel.cursor.document()->findBlock(sel.cursor.selectionStart());
        bool inIndentation = firstBlock.position() == sel.cursor.selectionStart();
        const auto createSplitSelection = [&] {
            QTextEdit::ExtraSelection newSelection;
            newSelection.cursor = QTextCursor(sel.cursor.document());
            newSelection.cursor.setPosition(firstNonWhitespacePos);
            newSelection.cursor.setPosition(lastNonWhitespacePos + 1, QTextCursor::KeepAnchor);
            newSelection.format = sel.format;
            splitSelections << newSelection;
        };
        for (int i = sel.cursor.selectionStart(); i < sel.cursor.selectionEnd(); ++i) {
            const QChar curChar = sel.cursor.document()->characterAt(i);
            if (!curChar.isSpace()) {
                if (firstNonWhitespacePos == -1)
                    firstNonWhitespacePos = i;
                lastNonWhitespacePos = i;
            }
            if (!inIndentation) {
                if (curChar == QChar::ParagraphSeparator)
                    inIndentation = true;
                continue;
            }
            if (curChar == QChar::ParagraphSeparator)
                continue;
            if (curChar.isSpace()) {
                if (firstNonWhitespacePos != -1) {
                    createSplitSelection();
                    firstNonWhitespacePos = -1;
                    lastNonWhitespacePos = -1;
                }
                split = true;
                continue;
            }
            inIndentation = false;
        }

        if (!split) {
            filtered << sel;
            continue;
        }

        if (firstNonWhitespacePos != -1)
            createSplitSelection();
        filtered << splitSelections;
    }
    return filtered;
}

bool CppEditorWidget::isInTestMode() const { return d->inTestMode; }

#ifdef WITH_TESTS
void CppEditorWidget::enableTestMode() { d->inTestMode = true; }
#endif

} // namespace CppEditor
