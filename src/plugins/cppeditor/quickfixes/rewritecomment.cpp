// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rewritecomment.h"

#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"

#include <cplusplus/ASTPath.h>
#include <cplusplus/declarationcomments.h>
#include <projectexplorer/editorconfiguration.h>
#include <texteditor/tabsettings.h>
#include <texteditor/textdocument.h>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#include <QtTest>
#endif

using namespace CPlusPlus;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

class ConvertCommentStyleOp : public CppQuickFixOperation
{
public:
    ConvertCommentStyleOp(const CppQuickFixInterface &interface, const QList<Token> &tokens,
                          Kind kind)
        : CppQuickFixOperation(interface),
        m_tokens(tokens),
        m_kind(kind),
        m_wasCxxStyle(m_kind == T_CPP_COMMENT || m_kind == T_CPP_DOXY_COMMENT),
        m_isDoxygen(m_kind == T_DOXY_COMMENT || m_kind == T_CPP_DOXY_COMMENT)
    {
        setDescription(m_wasCxxStyle ? Tr::tr("Convert Comment to C-Style")
                                     : Tr::tr("Convert Comment to C++-Style"));
    }

private:
    // Turns every line of a C-style comment into a C++-style comment and vice versa.
    // For C++ -> C, we use one /* */ comment block per line. However, doxygen
    // requires a single comment, so there we just replace the prefix with whitespace and
    // add the start and end comment in extra lines.
    // For cosmetic reasons, we offer some convenience functionality:
    //   - Turn /***** ... into ////// ... and vice versa
    //   - With C -> C++, remove leading asterisks.
    //   - With C -> C++, remove the first and last line of a block if they have no content
    //     other than the comment start and end characters.
    //   - With C++ -> C, try to align the end comment characters.
    // These are obviously heuristics; we do not guarantee perfect results for everybody.
    // We also don't second-guess the users's selection: E.g. if there is an empty
    // line between the tokens, then it's not the same doxygen comment, but we merge
    // it anyway in C++ to C mode.
    void perform() override
    {
        TranslationUnit * const tu = currentFile()->cppDocument()->translationUnit();
        const QString newCommentStart = getNewCommentStart();
        ChangeSet changeSet;
        int endCommentColumn = -1;
        const QChar oldFillChar = m_wasCxxStyle ? '/' : '*';
        const QChar newFillChar = m_wasCxxStyle ? '*' : '/';

        for (const Token &token : m_tokens) {
            const int startPos = tu->getTokenPositionInDocument(token, textDocument());
            const int endPos = tu->getTokenEndPositionInDocument(token, textDocument());

            if (m_wasCxxStyle && m_isDoxygen) {
                // Replace "///" characters with whitespace (to keep alignment).
                // The insertion of "/*" and "*/" is done once after the loop.
                changeSet.replace(startPos, startPos + 3, "   ");
                continue;
            }

            const QTextBlock firstBlock = textDocument()->findBlock(startPos);
            const QTextBlock lastBlock = textDocument()->findBlock(endPos);
            for (QTextBlock block = firstBlock; block.isValid() && block.position() <= endPos;
                 block = block.next()) {
                const QString &blockText = block.text();
                const int firstColumn = block == firstBlock ? startPos - block.position() : 0;
                const int endColumn = block == lastBlock ? endPos - block.position()
                                                         : block.length();

                // Returns true if the current line looks like "/********/" or "//////////",
                // as is often the case at the start and end of comment blocks.
                const auto fillChecker = [&] {
                    if (m_isDoxygen)
                        return false;
                    QString textToCheck = blockText;
                    if (block == firstBlock)
                        textToCheck.remove(0, 1);
                    if (block == lastBlock)
                        textToCheck.chop(block.length() - endColumn);
                    return Utils::allOf(textToCheck, [oldFillChar](const QChar &c)
                                        { return c == oldFillChar || c == ' ';
                                        }) && textToCheck.count(oldFillChar) > 2;
                };

                // Returns the index of the first character of actual comment content,
                // as opposed to visual stuff like slashes, stars or whitespace.
                const auto indexOfActualContent = [&] {
                    const int offset = block == firstBlock ? firstColumn + newCommentStart.length()
                                                           : firstColumn;

                    for (int i = offset, lastFillChar = -1; i < blockText.length(); ++i) {
                        if (blockText.at(i) == oldFillChar) {
                            lastFillChar = i;
                            continue;
                        }
                        if (!blockText.at(i).isSpace())
                            return lastFillChar + 1;
                    }
                    return -1;
                };

                if (fillChecker()) {
                    const QString replacement = QString(endColumn - 1 - firstColumn, newFillChar);
                    changeSet.replace(block.position() + firstColumn,
                                      block.position() + endColumn - 1,
                                      replacement);
                    if (m_wasCxxStyle) {
                        changeSet.replace(block.position() + firstColumn,
                                          block.position() + firstColumn + 1, "/");
                        changeSet.insert(block.position() + endColumn - 1, "*");
                        endCommentColumn = endColumn - 1;
                    }
                    continue;
                }

                // Remove leading noise or even the entire block, if applicable.
                const bool blockIsRemovable = (block == firstBlock || block == lastBlock)
                                              && firstBlock != lastBlock;
                const auto removeBlock = [&] {
                    changeSet.remove(block.position() + firstColumn, block.position() + endColumn);
                };
                const int contentIndex = indexOfActualContent();
                int removed = 0;
                if (contentIndex == -1) {
                    if (blockIsRemovable) {
                        removeBlock();
                        continue;
                    } else if (!m_wasCxxStyle) {
                        changeSet.replace(block.position() + firstColumn,
                                          block.position() + endColumn - 1, newCommentStart);
                        continue;
                    }
                } else if (block == lastBlock && contentIndex == endColumn - 1) {
                    if (blockIsRemovable) {
                        removeBlock();
                        break;
                    }
                } else {
                    changeSet.remove(block.position() + firstColumn,
                                     block.position() + firstColumn + contentIndex);
                    removed = contentIndex;
                }

                if (block == firstBlock) {
                    changeSet.replace(startPos, startPos + newCommentStart.length(),
                                      newCommentStart);
                } else {
                    // If the line starts with enough whitespace, replace it with the
                    // comment start characters, so we don't move the content to the right
                    // unnecessarily. Otherwise, insert the comment start characters.
                    if (blockText.startsWith(QString(newCommentStart.size() + removed + 1, ' '))) {
                        changeSet.replace(block.position(),
                                          block.position() + newCommentStart.length(),
                                          newCommentStart);
                    } else {
                        changeSet.insert(block.position(), newCommentStart);
                    }
                }

                if (block == lastBlock) {
                    if (m_wasCxxStyle) {
                        // This is for proper alignment of the end comment character.
                        if (endCommentColumn != -1) {
                            const int endCommentPos = block.position() + endCommentColumn;
                            if (endPos < endCommentPos)
                                changeSet.insert(endPos, QString(endCommentPos - endPos - 1, ' '));
                        }
                        changeSet.insert(endPos, " */");
                    } else {
                        changeSet.remove(endPos - 2, endPos);
                    }
                }
            }
        }

        if (m_wasCxxStyle && m_isDoxygen) {
            const int startPos = tu->getTokenPositionInDocument(m_tokens.first(), textDocument());
            const int endPos = tu->getTokenEndPositionInDocument(m_tokens.last(), textDocument());
            changeSet.insert(startPos, "/*!\n");
            changeSet.insert(endPos, "\n*/");
        }

        changeSet.apply(textDocument());
    }

    QString getNewCommentStart() const
    {
        if (m_wasCxxStyle) {
            if (m_isDoxygen)
                return "/*!";
            return "/*";
        }
        if (m_isDoxygen)
            return "//!";
        return "//";
    }

    const QList<Token> m_tokens;
    const Kind m_kind;
    const bool m_wasCxxStyle;
    const bool m_isDoxygen;
};

class MoveFunctionCommentsOp : public CppQuickFixOperation
{
public:
    enum class Direction { ToDecl, ToDef };
    MoveFunctionCommentsOp(const CppQuickFixInterface &interface, const Symbol *symbol,
                           const QList<Token> &commentTokens, Direction direction)
        : CppQuickFixOperation(interface), m_symbol(symbol), m_commentTokens(commentTokens)
    {
        setDescription(direction == Direction::ToDecl
                           ? Tr::tr("Move Function Documentation to Declaration")
                           : Tr::tr("Move Function Documentation to Definition"));
    }

private:
    void perform() override
    {
        const CppRefactoringFilePtr file = currentFile();
        const auto textDoc = const_cast<QTextDocument *>(file->document());
        const int pos = file->cppDocument()->translationUnit()->getTokenPositionInDocument(
            m_symbol->sourceLocation(), textDoc);
        QTextCursor cursor(textDoc);
        cursor.setPosition(pos);
        const CursorInEditor cursorInEditor(cursor, file->filePath(), editor(),
                                            editor()->textDocument());
        const auto callback = [symbolLoc = m_symbol->toLink(), comments = m_commentTokens, file]
            (const Link &link) {
                moveComments(file, link, symbolLoc, comments);
            };
        NonInteractiveFollowSymbolMarker niMarker;
        CppCodeModelSettings::setInteractiveFollowSymbol(false);
        CppModelManager::followSymbol(cursorInEditor, callback, true, false,
                                      FollowSymbolMode::Exact);
    }

    static void moveComments(
        const CppRefactoringFilePtr &sourceFile,
        const Link &targetLoc,
        const Link &symbolLoc,
        const QList<Token> &comments)
    {
        if (!targetLoc.hasValidTarget() || targetLoc.hasSameLocation(symbolLoc))
            return;

        CppRefactoringChanges changes(CppModelManager::snapshot());
        const CppRefactoringFilePtr targetFile
            = targetLoc.targetFilePath == symbolLoc.targetFilePath
                  ? sourceFile
                  : changes.cppFile(targetLoc.targetFilePath);
        const Document::Ptr &targetCppDoc = targetFile->cppDocument();
        const QList<AST *> targetAstPath = ASTPath(targetCppDoc)(
            targetLoc.targetLine, targetLoc.targetColumn + 1);
        if (targetAstPath.isEmpty())
            return;
        const AST *targetDeclAst = nullptr;
        for (auto it = std::next(std::rbegin(targetAstPath));
             it != std::rend(targetAstPath); ++it) {
            AST * const node = *it;
            if (node->asDeclaration()) {
                targetDeclAst = node;
                continue;
            }
            if (targetDeclAst)
                break;
        }
        if (!targetDeclAst)
            return;
        const int insertionPos = targetCppDoc->translationUnit()->getTokenPositionInDocument(
            targetDeclAst->firstToken(), targetFile->document());
        const TranslationUnit * const sourceTu = sourceFile->cppDocument()->translationUnit();
        const int sourceCommentStartPos = sourceTu->getTokenPositionInDocument(
            comments.first(), sourceFile->document());
        const int sourceCommentEndPos = sourceTu->getTokenEndPositionInDocument(
            comments.last(), sourceFile->document());

        // Manually adjust indentation, as both our built-in indenter and ClangFormat
        // are unreliable with regards to comment continuation lines.
        auto tabSettings = [](CppRefactoringFilePtr file) {
            if (auto editor = file->editor())
                return editor->textDocument()->tabSettings();
            return ProjectExplorer::actualTabSettings(file->filePath(), nullptr);
        };
        const TabSettings &sts = tabSettings(sourceFile);
        const TabSettings &tts = tabSettings(targetFile);
        const QTextBlock insertionBlock = targetFile->document()->findBlock(insertionPos);
        const int insertionColumn = tts.columnAt(insertionBlock.text(),
                                                 insertionPos - insertionBlock.position());
        const QTextBlock removalBlock = sourceFile->document()->findBlock(sourceCommentStartPos);
        const QTextBlock removalBlockEnd = sourceFile->document()->findBlock(sourceCommentEndPos);
        const int removalColumn = sts.columnAt(removalBlock.text(),
                                               sourceCommentStartPos - removalBlock.position());
        const int columnOffset = insertionColumn - removalColumn;
        QString functionDoc;
        if (columnOffset != 0) {
            for (QTextBlock block = removalBlock;
                 block.isValid() && block != removalBlockEnd.next();
                 block = block.next()) {
                QString text = block.text() + QChar::ParagraphSeparator;
                if (block == removalBlockEnd)
                    text = text.left(sourceCommentEndPos - block.position());
                if (block == removalBlock) {
                    text = text.mid(sourceCommentStartPos - block.position());
                } else {
                    int lineIndentColumn = sts.indentationColumn(text) + columnOffset;
                    text.replace(0,
                                 TabSettings::firstNonSpace(text),
                                 tts.indentationString(0, lineIndentColumn, 0, insertionBlock));
                }
                functionDoc += text;
            }
        } else {
            functionDoc = sourceFile->textOf(sourceCommentStartPos, sourceCommentEndPos);
        }

        // Remove comment plus leading and trailing whitespace, including trailing newline.
        const auto removeAtSource = [&](ChangeSet &changeSet) {
            int removalPos = sourceCommentStartPos;
            const QChar newline(QChar::ParagraphSeparator);
            while (true) {
                const int prev = removalPos - 1;
                if (prev < 0)
                    break;
                const QChar prevChar = sourceFile->charAt(prev);
                if (!prevChar.isSpace() || prevChar == newline)
                    break;
                removalPos = prev;
            }
            int removalEndPos = sourceCommentEndPos;
            while (true) {
                if (removalEndPos == sourceFile->document()->characterCount())
                    break;
                const QChar nextChar = sourceFile->charAt(removalEndPos);
                if (!nextChar.isSpace())
                    break;
                ++removalEndPos;
                if (nextChar == newline)
                    break;
            }
            changeSet.remove(removalPos, removalEndPos);
        };

        ChangeSet targetChangeSet;
        targetChangeSet.insert(insertionPos, functionDoc);
        targetChangeSet.insert(insertionPos, "\n");
        targetChangeSet.insert(insertionPos, QString(insertionColumn, ' '));
        if (targetFile == sourceFile)
            removeAtSource(targetChangeSet);
        const bool targetFileSuccess = targetFile->apply(targetChangeSet);
        if (targetFile == sourceFile || !targetFileSuccess)
            return;
        ChangeSet sourceChangeSet;
        removeAtSource(sourceChangeSet);
        sourceFile->apply(sourceChangeSet);
    }

    const Symbol * const m_symbol;
    const QList<Token> m_commentTokens;
};

//! Converts C-style to C++-style comments and vice versa
class ConvertCommentStyle : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject* createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface,
                 TextEditor::QuickFixOperations &result) override
    {
        // If there's a selection, then it must entirely consist of comment tokens.
        // If there's no selection, the cursor must be on a comment.
        const QList<Token> &cursorTokens = interface.currentFile()->tokensForCursor();
        if (cursorTokens.empty())
            return;
        if (!cursorTokens.front().isComment())
            return;

        // All tokens must be the same kind of comment, but we make an exception for doxygen comments
        // that start with "///", as these are often not intended to be doxygen. For our purposes,
        // we treat them as normal comments.
        const auto effectiveKind = [&interface](const Token &token) {
            if (token.kind() != T_CPP_DOXY_COMMENT)
                return token.kind();
            TranslationUnit * const tu = interface.currentFile()->cppDocument()->translationUnit();
            const int startPos = tu->getTokenPositionInDocument(token, interface.textDocument());
            const QString commentStart = interface.textAt(startPos, 3);
            return commentStart == "///" ? T_CPP_COMMENT : T_CPP_DOXY_COMMENT;
        };
        const Kind kind = effectiveKind(cursorTokens.first());
        for (int i = 1; i < cursorTokens.count(); ++i) {
            if (effectiveKind(cursorTokens.at(i)) != kind)
                return;
        }

        // Ok, all tokens are of same(ish) comment type, offer quickfix.
        result << new ConvertCommentStyleOp(interface, cursorTokens, kind);
    }
};

//! Moves function documentation between declaration and implementation.
class MoveFunctionComments : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject* createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface,
                 TextEditor::QuickFixOperations &result) override
    {
        const QList<AST *> &astPath = interface.path();
        if (astPath.isEmpty())
            return;
        const Symbol *symbol = nullptr;
        MoveFunctionCommentsOp::Direction direction = MoveFunctionCommentsOp::Direction::ToDecl;
        for (auto it = std::next(std::rbegin(astPath)); it != std::rend(astPath); ++it) {
            if (const auto func = (*it)->asFunctionDefinition()) {
                symbol = func->symbol;
                direction = MoveFunctionCommentsOp::Direction::ToDecl;
                break;
            }
            const auto decl = (*it)->asSimpleDeclaration();
            if (!decl || !decl->declarator_list)
                continue;
            for (auto it = decl->declarator_list->begin();
                 !symbol && it != decl->declarator_list->end(); ++it) {
                PostfixDeclaratorListAST * const funcDecls = (*it)->postfix_declarator_list;
                if (!funcDecls)
                    continue;
                for (auto it = funcDecls->begin(); it != funcDecls->end(); ++it) {
                    if (const auto func = (*it)->asFunctionDeclarator()) {
                        symbol = func->symbol;
                        direction = MoveFunctionCommentsOp::Direction::ToDef;
                        break;
                    }
                }
            }

        }
        if (!symbol)
            return;

        if (const QList<Token> commentTokens = commentsForDeclaration(
                symbol, *interface.textDocument(), interface.currentFile()->cppDocument());
            !commentTokens.isEmpty()) {
            result << new MoveFunctionCommentsOp(interface, symbol, commentTokens, direction);
        }
    }
};

#ifdef WITH_TESTS
using namespace Tests;

class ConvertCommentStyleTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("expectedOutput");

        QTest::newRow("C -> C++ / no selection / single line") << R"(
int var1;
/* Other comment, unaffected */
/* Our @comment */
/* Another unaffected comment */
int var2;)" << R"(
int var1;
/* Other comment, unaffected */
// Our comment
/* Another unaffected comment */
int var2;)";

        QTest::newRow("C -> C++ / no selection / multi-line / preserved header and footer") << R"(
/****************************************************
 * some info
 * more @info
 ***************************************************/)" << R"(
/////////////////////////////////////////////////////
// some info
// more info
/////////////////////////////////////////////////////)";

        QTest::newRow("C -> C++ / no selection / multi-line / non-preserved header and footer") << R"(
/*
 * some info
 * more @info
 */)" << R"(
// some info
// more info
)";

        QTest::newRow("C -> C++ / no selection / qdoc") << R"(
/*!
    \qmlproperty string Type::element.name
    \qmlproperty int Type::element.id

    \brief Holds the @element name and id.
*/)" << R"(
//! \qmlproperty string Type::element.name
//! \qmlproperty @int Type::element.id
//!
//! \brief Holds the element name and id.
)";

        QTest::newRow("C -> C++ / no selection / doxygen") << R"(
/*! \class Test
    \brief A test class.

    A more detailed @class description.
*/)" << R"(
//! \class Test
//! \brief A test class.
//!
//! A more detailed class description.
)";

        QTest::newRow("C -> C++ / selection / single line") << R"(
int var1;
/* Other comment, unaffected */
@{start}/* Our comment */@{end}
/* Another unaffected comment */
int var2;)" << R"(
int var1;
/* Other comment, unaffected */
// Our comment
/* Another unaffected comment */
int var2;)";

        QTest::newRow("C -> C++ / selection / multi-line / preserved header and footer") << R"(
/****************************************************
 * @{start}some info
 * more info@{end}
 ***************************************************/)" << R"(
/////////////////////////////////////////////////////
// some info
// more info
/////////////////////////////////////////////////////)";

        QTest::newRow("C -> C++ / selection / multi-line / non-preserved header and footer") << R"(
/*@{start}
 * some in@{end}fo
 * more info
 */)" << R"(
// some info
// more info
)";

        QTest::newRow("C -> C++ / selection / qdoc") << R"(
/*!@{start}
    \qmlproperty string Type::element.name
    \qmlproperty int Type::element.id

    \brief Holds the element name and id.
*/@{end})" << R"(
//! \qmlproperty string Type::element.name
//! \qmlproperty int Type::element.id
//!
//! \brief Holds the element name and id.
)";

        QTest::newRow("C -> C++ / selection / doxygen") << R"(
/** Expand envi@{start}ronment variables in a string.
 *
 * Environment variables are accepted in the @{end}following forms:
 * $SOMEVAR, ${SOMEVAR} on Unix and %SOMEVAR% on Windows.
 * No escapes and quoting are supported.
 * If a variable is not found, it is not substituted.
 */)" << R"(
//! Expand environment variables in a string.
//!
//! Environment variables are accepted in the following forms:
//! $SOMEVAR, ${SOMEVAR} on Unix and %SOMEVAR% on Windows.
//! No escapes and quoting are supported.
//! If a variable is not found, it is not substituted.
)";

        QTest::newRow("C -> C++ / selection / multiple comments") << R"(
@{start}/* Affected comment */
/* Another affected comment */
/* A third affected comment */@{end}
/* An unaffected comment */)" << R"(
// Affected comment
// Another affected comment
// A third affected comment
/* An unaffected comment */)";

        // FIXME: Remove adjacent newline along with last block
        // FIXME: Use CppRefactoringFile to auto-indent continuation lines?
        QTest::newRow("C -> C++, indented") << R"(
struct S {
    /*
     * @This is an
     * indented comment.
     */
    void func();
)" << R"(
struct S {
    // This is an
// indented comment.

    void func();
)";

        QTest::newRow("C++ -> C / no selection / single line") << R"(
// Other comment, unaffected
// Our @comment
// Another unaffected comment)" << R"(
// Other comment, unaffected
/* Our comment */
// Another unaffected comment)";

        QTest::newRow("C++ -> C / selection / single line") << R"(
// Other comment, unaffected
@{start}// Our comment@{end}
// Another unaffected comment)" << R"(
// Other comment, unaffected
/* Our comment */
// Another unaffected comment)";

        QTest::newRow("C++ -> C / selection / multi-line / preserved header and footer") << R"(
@{start}/////////////////////////////////////////////////////
// some info
// more info
/////////////////////////////////////////////////////@{end})" << R"(
/****************************************************/
/* some info                                        */
/* more info                                        */
/****************************************************/)";

        QTest::newRow("C++ -> C / selection / qdoc") << R"(
@{start}//! \qmlproperty string Type::element.name
//! \qmlproperty int Type::element.id
//!
//! \brief Holds the element name and id.@{end}
)" << R"(
/*!
    \qmlproperty string Type::element.name
    \qmlproperty int Type::element.id

    \brief Holds the element name and id.
*/
)";

        QTest::newRow("C++ -> C / selection / doxygen") << R"(
@{start}//! \class Test
//! \brief A test class.
//!
//! A more detailed class description.@{end}
)" << R"(
/*!
    \class Test
    \brief A test class.

    A more detailed class description.
*/
)";
    }

    void test()
    {
        QFETCH(QString, input);
        QFETCH(QString, expectedOutput);

        ConvertCommentStyle factory;
        QuickFixOperationTest(
            {CppTestDocument::create("file.h", input.toUtf8(), expectedOutput.toUtf8())},
            &factory);
    }
};

class MoveFunctionCommentsTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArrayList>("headers");
        QTest::addColumn<QByteArrayList>("sources");

        const QByteArrayList headersFuncDecl2Def{R"(
// Function comment
void @aFunction();
)", R"(
void aFunction();
)"};
        const QByteArrayList sourcesFuncDecl2Def{R"(
#include "file.h"

void aFunction() {}
)", R"(
#include "file.h"

// Function comment
void aFunction() {}
)"};
        QTest::newRow("function: from decl to def") << headersFuncDecl2Def << sourcesFuncDecl2Def;

        const QByteArrayList headersFuncDef2Decl{R"(
void aFunction();
)", R"(
/* function */
/* comment */
void aFunction();
)"};
        const QByteArrayList sourcesFuncDef2Decl{R"(
#include "file.h"

/* function */
/* comment */
void a@Function() {}
)", R"(
#include "file.h"

void aFunction() {}
)"};
        QTest::newRow("function: from def to decl") << headersFuncDef2Decl << sourcesFuncDef2Decl;

        const QByteArrayList headersFuncNoDef{R"(
// Function comment
void @aFunction();
)", R"(
// Function comment
void aFunction();
)"};
        QTest::newRow("function: no def") << headersFuncNoDef << QByteArrayList();

        const QByteArrayList headersFuncNoDecl{R"(
// Function comment
inline void @aFunction() {}
)", R"(
// Function comment
inline void aFunction() {}
)"};
        QTest::newRow("function: no decl") << headersFuncNoDecl << QByteArrayList();

        const QByteArrayList headersFuncTemplateDecl2Def{R"(
// Function comment
template<typename T> T @aFunction();

template<typename T> inline T aFunction() { return T(); }
)", R"(
template<typename T> T aFunction();

// Function comment
template<typename T> inline T aFunction() { return T(); }
)"};
        QTest::newRow("function template: from decl to def") << headersFuncTemplateDecl2Def
                                                             << QByteArrayList();

        const QByteArrayList headersFuncTemplateDef2Decl{R"(
template<typename T> T aFunction();

// Function comment
template<typename T> inline T @aFunction() { return T(); }
)", R"(
// Function comment
template<typename T> T aFunction();

template<typename T> inline T aFunction() { return T(); }
)"};
        QTest::newRow("function template: from def to decl") << headersFuncTemplateDef2Decl
                                                             << QByteArrayList();

        const QByteArrayList headersMemberDecl2Def{R"(
class C {
    /**
      * \brief Foo::aMember
      */
    void @aMember();
)", R"(
class C {
    void aMember();
)"};
        const QByteArrayList sourcesMemberDecl2Def{R"(
#include "file.h"

void C::aMember() {}
)", R"(
#include "file.h"

/**
  * \brief Foo::aMember
  */
void C::aMember() {}
)"};
        QTest::newRow("member function: from decl to def") << headersMemberDecl2Def
                                                           << sourcesMemberDecl2Def;

        const QByteArrayList headersMemberDef2Decl{R"(
class C {
    void aMember();
)", R"(
class C {
    /**
      * \brief Foo::aMember
      */
    void aMember();
)"};
        const QByteArrayList sourcesMemberDef2Decl{R"(
#include "file.h"

/**
  * \brief Foo::aMember
  */
void C::aMember() {@}
)", R"(
#include "file.h"

void C::aMember() {}
)"};
        QTest::newRow("member function: from def to decl") << headersMemberDef2Decl
                                                           << sourcesMemberDef2Decl;
    }

    void test()
    {
        QFETCH(QByteArrayList, headers);
        QFETCH(QByteArrayList, sources);

        QList<TestDocumentPtr> documents;
        QCOMPARE(headers.size(), 2);
        documents << CppTestDocument::create("file.h", headers.at(0), headers.at(1));
        if (!sources.isEmpty()) {
            QCOMPARE(sources.size(), 2);
            documents << CppTestDocument::create("file.cpp", sources.at(0), sources.at(1));
        }
        MoveFunctionComments factory;
        QByteArray failMessage;
        if (QByteArray(QTest::currentDataTag()) == "function template: from def to decl")
            failMessage = "decl/def switch doesn't work for templates";
        QuickFixOperationTest(documents, &factory, {}, {}, failMessage);
    }
};

QObject * ConvertCommentStyle::createTest() { return new ConvertCommentStyleTest; }
QObject * MoveFunctionComments::createTest() { return new MoveFunctionCommentsTest; }

#endif
} // namespace

void registerRewriteCommentQuickfixes()
{
    CppQuickFixFactory::registerFactory<ConvertCommentStyle>();
    CppQuickFixFactory::registerFactory<MoveFunctionComments>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <rewritecomment.moc>
#endif
