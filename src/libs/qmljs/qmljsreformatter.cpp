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

#include "qmljsreformatter.h"

#include "qmljscodeformatter.h"
#include "qmljsscanner.h"
#include "parser/qmljsast_p.h"
#include "parser/qmljsastvisitor_p.h"

#include <QCoreApplication>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextCursor>

#include <limits>

using namespace QmlJS;
using namespace QmlJS::AST;

namespace {

class SimpleFormatter : public QtStyleCodeFormatter
{
protected:
    class FormatterData : public QTextBlockUserData
    {
    public:
        FormatterData(const BlockData &data) : data(data) {}
        BlockData data;
    };

    virtual void saveBlockData(QTextBlock *block, const BlockData &data) const
    {
        block->setUserData(new FormatterData(data));
    }

    virtual bool loadBlockData(const QTextBlock &block, BlockData *data) const
    {
        if (!block.userData())
            return false;

        *data = static_cast<FormatterData *>(block.userData())->data;
        return true;
    }

    virtual void saveLexerState(QTextBlock *block, int state) const
    {
        block->setUserState(state);
    }

    virtual int loadLexerState(const QTextBlock &block) const
    {
        return block.userState();
    }
};

class Rewriter : protected Visitor
{
    Document::Ptr _doc;
    QString _result;
    QString _line;
    class Split {
    public:
        int offset;
        qreal badness;
    };
    QList<Split> _possibleSplits;
    QTextDocument _resultDocument;
    SimpleFormatter _formatter;
    int _indent;
    int _nextComment;
    int _lastNewlineOffset;
    bool _hadEmptyLine;
    int _binaryExpDepth;

public:
    Rewriter(Document::Ptr doc)
        : _doc(doc)
    {
    }

    QString operator()(Node *node)
    {
        Q_ASSERT(node == _doc->ast()); // comment handling fails otherwise

        _result.reserve(_doc->source().size());
        _line.clear();
        _possibleSplits.clear();
        _indent = 0;
        _nextComment = 0;
        _lastNewlineOffset = -1;
        _hadEmptyLine = false;
        _binaryExpDepth = 0;

        accept(node);

        // emit the final comments
        const QList<SourceLocation> &comments = _doc->engine()->comments();
        for (; _nextComment < comments.size(); ++_nextComment) {
            outComment(comments.at(_nextComment));
        }

        // ensure good ending
        if (!_result.endsWith(QLatin1String("\n\n")) || !_line.isEmpty())
            newLine();

        return _result;
    }

protected:
    void accept(Node *node)
    {
        Node::accept(node, this);
    }

    void acceptIndented(Node *node)
    {
        accept(node);
    }

    void lnAcceptIndented(Node *node)
    {
        newLine();
        accept(node);
    }

    void out(const char *str, const SourceLocation &lastLoc = SourceLocation())
    {
        out(QString::fromLatin1(str), lastLoc);
    }

    void outComment(const SourceLocation &commentLoc)
    {
        SourceLocation fixedLoc = commentLoc;
        fixCommentLocation(fixedLoc);
        if (precededByEmptyLine(fixedLoc))
            newLine();
        out(toString(fixedLoc)); // don't use the sourceloc overload here
        if (followedByNewLine(fixedLoc))
            newLine();
        else
            out(" ");
    }

    void out(const QString &str, const SourceLocation &lastLoc = SourceLocation())
    {
        if (lastLoc.isValid()) {
            QList<SourceLocation> comments = _doc->engine()->comments();
            for (; _nextComment < comments.size(); ++_nextComment) {
                SourceLocation commentLoc = comments.at(_nextComment);
                if (commentLoc.end() > lastLoc.end())
                    break;

                outComment(commentLoc);
            }
        }

        QStringList lines = str.split(QLatin1Char('\n'));
        for (int i = 0; i < lines.size(); ++i) {
            _line += lines.at(i);
            if (i != lines.size() - 1)
                newLine();
        }
        _hadEmptyLine = false;
    }

    QString toString(const SourceLocation &loc)
    {
        return _doc->source().mid(loc.offset, loc.length);
    }

    void out(const SourceLocation &loc)
    {
        if (!loc.isValid())
            return;
        out(toString(loc), loc);
    }

    int tryIndent(const QString &line)
    {
        // append the line to the text document
        QTextCursor cursor(&_resultDocument);
        cursor.movePosition(QTextCursor::End);
        int cursorStartLinePos = cursor.position();
        cursor.insertText(line);

        // get the expected indentation
        QTextBlock last = _resultDocument.lastBlock();
        _formatter.updateStateUntil(last);
        int indent = _formatter.indentFor(last);

        // remove the line again
        cursor.setPosition(cursorStartLinePos);
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();

        return indent;
    }

    void finishLine()
    {
        // remove trailing spaces
        int len = _line.size();
        while (len > 0 && _line.at(len - 1).isSpace())
            --len;
        _line.resize(len);

        _line += QLatin1Char('\n');

        _result += _line;
        QTextCursor cursor(&_resultDocument);
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(_line);

        _line = QString(_indent, QLatin1Char(' '));
    }

    class BestSplit {
    public:
        QStringList lines;
        qreal badnessFromSplits;

        qreal badness()
        {
            const int maxLineLength = 80;
            const int strongMaxLineLength = 100;
            const int minContentLength = 10;

            qreal result = badnessFromSplits;
            foreach (const QString &line, lines) {
                // really long lines should be avoided at all cost
                if (line.size() > strongMaxLineLength)
                    result += 50 + (line.size() - strongMaxLineLength);
                // having long lines is bad
                else if (line.size() > maxLineLength) {
                    result += 3 + (line.size() - maxLineLength);
                }
                // and even ok-sized lines should have a cost
                else {
                    result += 1;
                }

                // having lines with little content is bad
                const int contentSize = line.trimmed().size();
                if (contentSize < minContentLength)
                    result += 0.5;
            }
            return result;
        }
    };

    void adjustIndent(QString *line, QList<Split> *splits, int indent)
    {
        int startSpaces = 0;
        while (startSpaces < line->size() && line->at(startSpaces).isSpace())
            ++startSpaces;

        line->replace(0, startSpaces, QString(indent, QLatin1Char(' ')));
        for (int i = 0; i < splits->size(); ++i) {
            (*splits)[i].offset = splits->at(i).offset - startSpaces + indent;
        }
    }

    BestSplit computeBestSplits(QStringList context, QString line, QList<Split> possibleSplits)
    {
        BestSplit result;

        result.badnessFromSplits = 0;
        result.lines = QStringList(line);

        //qDebug() << "trying to split" << line << possibleSplits << context;

        // try to split at every possible position
        for (int i = possibleSplits.size() - 1; i >= 0; --i) {
            const int splitPos = possibleSplits.at(i).offset;
            const QString newContextLine = line.left(splitPos);
            QStringList newContext = context;
            newContext += newContextLine;
            const QString restLine = line.mid(splitPos);
            if (restLine.trimmed().isEmpty())
                continue;

            // the extra space is to avoid // comments sticking to the 0 offset
            QString indentLine = newContext.join(QLatin1String("\n")) + QLatin1String("\n ") + restLine;
            int indent = tryIndent(indentLine);

            QList<Split> newSplits = possibleSplits.mid(i + 1);
            QString newSplitLine = restLine;
            adjustIndent(&newSplitLine, &newSplits, indent);

            for (int j = 0; j < newSplits.size(); ++j)
                newSplits[j].offset = newSplits.at(j).offset - splitPos;

            BestSplit nested = computeBestSplits(newContext, newSplitLine, newSplits);

            nested.lines.prepend(newContextLine);
            nested.badnessFromSplits += possibleSplits.at(i).badness;
            if (nested.badness() < result.badness())
                result = nested;
        }

        return result;
    }

    void newLine()
    {
        // if preceded by a newline, it's an empty line!
        _hadEmptyLine = _line.trimmed().isEmpty();

        // if the preceding line wasn't empty, reindent etc.
        if (!_hadEmptyLine) {
            int indentStart = 0;
            while (indentStart < _line.size() && _line.at(indentStart).isSpace())
                ++indentStart;

            _indent = tryIndent(_line);
            adjustIndent(&_line, &_possibleSplits, _indent);

            // maybe make multi-line?
            BestSplit split = computeBestSplits(QStringList(), _line, _possibleSplits);
            if (!split.lines.isEmpty() && split.lines.size() > 1) {
                for (int i = 0; i < split.lines.size(); ++i) {
                    _line = split.lines.at(i);
                    if (i != split.lines.size() - 1)
                        finishLine();
                }
            }
        }

        finishLine();
        _possibleSplits.clear();
    }

    void requireEmptyLine()
    {
        while (!_hadEmptyLine)
            newLine();
    }

    bool acceptBlockOrIndented(Node *ast, bool finishWithSpaceOrNewline = false)
    {
        if (cast<Block *>(ast)) {
            out(" ");
            accept(ast);
            if (finishWithSpaceOrNewline)
                out(" ");
            return true;
        } else {
            lnAcceptIndented(ast);
            if (finishWithSpaceOrNewline)
                newLine();
            return false;
        }
    }

    bool followedByNewLine(const SourceLocation &loc)
    {
        const QString &source = _doc->source();
        int i = loc.end();
        for (; i < source.size() && source.at(i).isSpace(); ++i) {
            if (source.at(i) == QLatin1Char('\n'))
                return true;
        }
        return false;
    }

    bool precededByEmptyLine(const SourceLocation &loc)
    {
        const QString &source = _doc->source();
        int i = loc.offset;

        // expect spaces and \n, twice
        bool first = true;
        for (--i;
             i >= 0 && source.at(i).isSpace();
             --i) {

            if (source.at(i) == QLatin1Char('\n')) {
                if (first)
                    first = false;
                else
                    return true;
            }
        }
        return false;
    }

    bool firstOnLine()
    {
        foreach (const QChar &c, _line) {
            if (!c.isSpace())
                return false;
        }
        return true;
    }

    void addPossibleSplit(qreal badness, int offset = 0)
    {
        Split s;
        s.badness = badness;
        s.offset = _line.size() + offset;
        _possibleSplits += s;
    }

    void fixCommentLocation(SourceLocation &loc)
    {
        loc.offset -= 2;
        loc.startColumn -= 2;
        loc.length += 2;
        if (_doc->source().mid(loc.offset, 2) == QLatin1String("/*"))
            loc.length += 2;
    }

    virtual bool preVisit(Node *ast)
    {
        SourceLocation firstLoc;
        if (ExpressionNode *expr = ast->expressionCast())
            firstLoc = expr->firstSourceLocation();
        else if (Statement *stmt = ast->statementCast())
            firstLoc = stmt->firstSourceLocation();
        else if (UiObjectMember *mem = ast->uiObjectMemberCast())
            firstLoc = mem->firstSourceLocation();
        else if (UiImport *import = cast<UiImport *>(ast))
            firstLoc = import->firstSourceLocation();

        if (firstLoc.isValid() && int(firstLoc.offset) != _lastNewlineOffset) {
            _lastNewlineOffset = firstLoc.offset;

            if (precededByEmptyLine(firstLoc) && !_result.endsWith(QLatin1String("\n\n")))
                newLine();
        }

        return true;
    }

    virtual void postVisit(Node *ast)
    {
        SourceLocation lastLoc;
        if (ExpressionNode *expr = ast->expressionCast())
            lastLoc = expr->lastSourceLocation();
        else if (Statement *stmt = ast->statementCast())
            lastLoc = stmt->lastSourceLocation();
        else if (UiObjectMember *mem = ast->uiObjectMemberCast())
            lastLoc = mem->lastSourceLocation();
        else if (UiImport *import = cast<UiImport *>(ast))
            lastLoc = import->lastSourceLocation();

        if (lastLoc.isValid()) {
            const QList<SourceLocation> &comments = _doc->engine()->comments();

            // preserve trailing comments
            for (; _nextComment < comments.size(); ++_nextComment) {
                SourceLocation nextCommentLoc = comments.at(_nextComment);
                if (nextCommentLoc.startLine != lastLoc.startLine)
                    break;
                fixCommentLocation(nextCommentLoc);

                // there must only be whitespace between lastLoc and the comment
                bool commentFollows = true;
                for (quint32 i = lastLoc.end(); i < nextCommentLoc.begin(); ++i) {
                    if (!_doc->source().at(i).isSpace()) {
                        commentFollows = false;
                        break;
                    }
                }
                if (!commentFollows)
                    break;

                out(" ");
                out(toString(nextCommentLoc));
            }
        }
    }

    virtual bool visit(UiImport *ast)
    {
        out("import ", ast->importToken);
        if (!ast->fileName.isNull())
            out(QString::fromLatin1("\"%1\"").arg(ast->fileName.toString()));
        else
            accept(ast->importUri);
        if (ast->versionToken.isValid()) {
            out(" ");
            out(ast->versionToken);
        }
        if (!ast->importId.isNull()) {
            out(" as ", ast->asToken);
            out(ast->importIdToken);
        }
        return false;
    }

    virtual bool visit(UiObjectDefinition *ast)
    {
        accept(ast->qualifiedTypeNameId);
        out(" ");
        accept(ast->initializer);
        return false;
    }

    virtual bool visit(UiObjectInitializer *ast)
    {
        out(ast->lbraceToken);
        if (ast->members)
            lnAcceptIndented(ast->members);
        newLine();
        out(ast->rbraceToken);
        return false;
    }

    virtual bool visit(UiParameterList *list)
    {
        for (UiParameterList *it = list; it; it = it->next) {
            out(it->propertyTypeToken);
            out(" ");
            out(it->identifierToken);
            if (it->next)
                out(", ", it->commaToken);
        }
        return false;
    }

    virtual bool visit(UiPublicMember *ast)
    {
        if (ast->type == UiPublicMember::Property) {
            if (ast->isDefaultMember)
                out("default ", ast->defaultToken);
            out("property ", ast->propertyToken);
            if (!ast->typeModifier.isNull()) {
                out(ast->typeModifierToken);
                out("<");
                out(ast->typeToken);
                out(">");
            } else {
                out(ast->typeToken);
            }
            out(" ");
            out(ast->identifierToken);
            if (ast->statement) {
                out(": ", ast->colonToken);
                accept(ast->statement);
            } else if (ast->binding) {
                out(": ", ast->colonToken);
                accept(ast->binding);
            }
        } else { // signal
            out("signal ");
            out(ast->identifierToken);
            if (ast->parameters) {
                out("(");
                accept(ast->parameters);
                out(")");
            }
        }
        return false;
    }

    virtual bool visit(UiObjectBinding *ast)
    {
        if (ast->hasOnToken) {
            accept(ast->qualifiedTypeNameId);
            out(" on ");
            accept(ast->qualifiedId);
        } else {
            accept(ast->qualifiedId);
            out(": ", ast->colonToken);
            accept(ast->qualifiedTypeNameId);
        }
        out(" ");
        accept(ast->initializer);
        return false;
    }

    virtual bool visit(UiScriptBinding *ast)
    {
        accept(ast->qualifiedId);
        out(": ", ast->colonToken);
        accept(ast->statement);
        return false;
    }

    virtual bool visit(UiArrayBinding *ast)
    {
        accept(ast->qualifiedId);
        out(ast->colonToken);
        out(" ");
        out(ast->lbracketToken);
        lnAcceptIndented(ast->members);
        newLine();
        out(ast->rbracketToken);
        return false;
    }

    virtual bool visit(ThisExpression *ast) { out(ast->thisToken); return true; }
    virtual bool visit(NullExpression *ast) { out(ast->nullToken); return true; }
    virtual bool visit(TrueLiteral *ast) { out(ast->trueToken); return true; }
    virtual bool visit(FalseLiteral *ast) { out(ast->falseToken); return true; }
    virtual bool visit(IdentifierExpression *ast) { out(ast->identifierToken); return true; }
    virtual bool visit(StringLiteral *ast) { out(ast->literalToken); return true; }
    virtual bool visit(NumericLiteral *ast) { out(ast->literalToken); return true; }
    virtual bool visit(RegExpLiteral *ast) { out(ast->literalToken); return true; }

    virtual bool visit(ArrayLiteral *ast)
    {
        out(ast->lbracketToken);
        if (ast->elements)
            accept(ast->elements);
        if (ast->elements && ast->elision)
            out(", ", ast->commaToken);
        if (ast->elision)
            accept(ast->elision);
        out(ast->rbracketToken);
        return false;
    }

    virtual bool visit(ObjectLiteral *ast)
    {
        out(ast->lbraceToken);
        lnAcceptIndented(ast->properties);
        newLine();
        out(ast->rbraceToken);
        return false;
    }

    virtual bool visit(ElementList *ast)
    {
        for (ElementList *it = ast; it; it = it->next) {
            if (it->elision)
                accept(it->elision);
            if (it->elision && it->expression)
                out(", ");
            if (it->expression)
                accept(it->expression);
            if (it->next)
                out(", ", ast->commaToken);
        }
        return false;
    }

    virtual bool visit(PropertyNameAndValueList *ast)
    {
        for (PropertyNameAndValueList *it = ast; it; it = it->next) {
            accept(it->name);
            out(": ", ast->colonToken);
            accept(it->value);
            if (it->next) {
                out(",", ast->commaToken); // always invalid?
                newLine();
            }
        }
        return false;
    }

    virtual bool visit(NestedExpression *ast)
    {
        out(ast->lparenToken);
        accept(ast->expression);
        out(ast->rparenToken);
        return false;
    }

    virtual bool visit(IdentifierPropertyName *ast) { out(ast->id.toString()); return true; }
    virtual bool visit(StringLiteralPropertyName *ast) { out(ast->id.toString()); return true; }
    virtual bool visit(NumericLiteralPropertyName *ast) { out(QString::number(ast->id)); return true; }

    virtual bool visit(ArrayMemberExpression *ast)
    {
        accept(ast->base);
        out(ast->lbracketToken);
        accept(ast->expression);
        out(ast->rbracketToken);
        return false;
    }

    virtual bool visit(FieldMemberExpression *ast)
    {
        accept(ast->base);
        out(ast->dotToken);
        out(ast->identifierToken);
        return false;
    }

    virtual bool visit(NewMemberExpression *ast)
    {
        out("new ", ast->newToken);
        accept(ast->base);
        out(ast->lparenToken);
        accept(ast->arguments);
        out(ast->rparenToken);
        return false;
    }

    virtual bool visit(NewExpression *ast)
    {
        out("new ", ast->newToken);
        accept(ast->expression);
        return false;
    }

    virtual bool visit(CallExpression *ast)
    {
        accept(ast->base);
        out(ast->lparenToken);
        addPossibleSplit(0);
        accept(ast->arguments);
        out(ast->rparenToken);
        return false;
    }

    virtual bool visit(PostIncrementExpression *ast)
    {
        accept(ast->base);
        out(ast->incrementToken);
        return false;
    }

    virtual bool visit(PostDecrementExpression *ast)
    {
        accept(ast->base);
        out(ast->decrementToken);
        return false;
    }

    virtual bool visit(PreIncrementExpression *ast)
    {
        out(ast->incrementToken);
        accept(ast->expression);
        return false;
    }

    virtual bool visit(PreDecrementExpression *ast)
    {
        out(ast->decrementToken);
        accept(ast->expression);
        return false;
    }

    virtual bool visit(DeleteExpression *ast)
    {
        out("delete ", ast->deleteToken);
        accept(ast->expression);
        return false;
    }

    virtual bool visit(VoidExpression *ast)
    {
        out("void ", ast->voidToken);
        accept(ast->expression);
        return false;
    }

    virtual bool visit(TypeOfExpression *ast)
    {
        out("typeof ", ast->typeofToken);
        accept(ast->expression);
        return false;
    }

    virtual bool visit(UnaryPlusExpression *ast)
    {
        out(ast->plusToken);
        accept(ast->expression);
        return false;
    }

    virtual bool visit(UnaryMinusExpression *ast)
    {
        out(ast->minusToken);
        accept(ast->expression);
        return false;
    }

    virtual bool visit(TildeExpression *ast)
    {
        out(ast->tildeToken);
        accept(ast->expression);
        return false;
    }

    virtual bool visit(NotExpression *ast)
    {
        out(ast->notToken);
        accept(ast->expression);
        return false;
    }

    virtual bool visit(BinaryExpression *ast)
    {
        ++_binaryExpDepth;
        accept(ast->left);

        // in general, avoid splitting at the operator
        // but && and || are ok
        qreal splitBadness = 30;
        if (ast->op == QSOperator::And
                || ast->op == QSOperator::Or)
            splitBadness = 0;
        addPossibleSplit(splitBadness);

        out(" ");
        out(ast->operatorToken);
        out(" ");
        accept(ast->right);
        --_binaryExpDepth;
        return false;
    }

    virtual bool visit(ConditionalExpression *ast)
    {
        accept(ast->expression);
        out(" ? ", ast->questionToken);
        accept(ast->ok);
        out(" : ", ast->colonToken);
        accept(ast->ko);
        return false;
    }

    virtual bool visit(Block *ast)
    {
        out(ast->lbraceToken);
        lnAcceptIndented(ast->statements);
        newLine();
        out(ast->rbraceToken);
        return false;
    }

    virtual bool visit(VariableStatement *ast)
    {
        out("var ", ast->declarationKindToken);
        accept(ast->declarations);
        return false;
    }

    virtual bool visit(VariableDeclaration *ast)
    {
        out(ast->identifierToken);
        if (ast->expression) {
            out(" = ");
            accept(ast->expression);
        }
        return false;
    }

    virtual bool visit(EmptyStatement *ast)
    {
        out(ast->semicolonToken);
        return false;
    }

    virtual bool visit(IfStatement *ast)
    {
        out(ast->ifToken);
        out(" ");
        out(ast->lparenToken);
        accept(ast->expression);
        out(ast->rparenToken);
        acceptBlockOrIndented(ast->ok, ast->ko);
        if (ast->ko) {
            out(ast->elseToken);
            if (cast<Block *>(ast->ko) || cast<IfStatement *>(ast->ko)) {
                out(" ");
                accept(ast->ko);
            } else {
                lnAcceptIndented(ast->ko);
            }
        }
        return false;
    }

    virtual bool visit(DoWhileStatement *ast)
    {
        out(ast->doToken);
        acceptBlockOrIndented(ast->statement, true);
        out(ast->whileToken);
        out(" ");
        out(ast->lparenToken);
        accept(ast->expression);
        out(ast->rparenToken);
        return false;
    }

    virtual bool visit(WhileStatement *ast)
    {
        out(ast->whileToken);
        out(" ");
        out(ast->lparenToken);
        accept(ast->expression);
        out(ast->rparenToken);
        acceptBlockOrIndented(ast->statement);
        return false;
    }

    virtual bool visit(ForStatement *ast)
    {
        out(ast->forToken);
        out(" ");
        out(ast->lparenToken);
        accept(ast->initialiser);
        out("; ", ast->firstSemicolonToken);
        accept(ast->condition);
        out("; ", ast->secondSemicolonToken);
        accept(ast->expression);
        out(ast->rparenToken);
        acceptBlockOrIndented(ast->statement);
        return false;
    }

    virtual bool visit(LocalForStatement *ast)
    {
        out(ast->forToken);
        out(" ");
        out(ast->lparenToken);
        out(ast->varToken);
        out(" ");
        accept(ast->declarations);
        out("; ", ast->firstSemicolonToken);
        accept(ast->condition);
        out("; ", ast->secondSemicolonToken);
        accept(ast->expression);
        out(")", ast->rparenToken);
        acceptBlockOrIndented(ast->statement);
        return false;
    }

    virtual bool visit(ForEachStatement *ast)
    {
        out(ast->forToken);
        out(" ");
        out(ast->lparenToken);
        accept(ast->initialiser);
        out(" in ", ast->inToken);
        accept(ast->expression);
        out(ast->rparenToken);
        acceptBlockOrIndented(ast->statement);
        return false;
    }

    virtual bool visit(LocalForEachStatement *ast)
    {
        out(ast->forToken);
        out(" ");
        out(ast->lparenToken);
        out(ast->varToken);
        out(" ");
        accept(ast->declaration);
        out(" in ", ast->inToken);
        accept(ast->expression);
        out(ast->rparenToken);
        acceptBlockOrIndented(ast->statement);
        return false;
    }

    virtual bool visit(ContinueStatement *ast)
    {
        out(ast->continueToken);
        if (!ast->label.isNull()) {
            out(" ");
            out(ast->identifierToken);
        }
        return false;
    }

    virtual bool visit(BreakStatement *ast)
    {
        out(ast->breakToken);
        if (!ast->label.isNull()) {
            out(" ");
            out(ast->identifierToken);
        }
        return false;
    }

    virtual bool visit(ReturnStatement *ast)
    {
        out(ast->returnToken);
        if (ast->expression) {
            out(" ");
            accept(ast->expression);
        }
        return false;
    }

    virtual bool visit(ThrowStatement *ast)
    {
        out(ast->throwToken);
        if (ast->expression) {
            out(" ");
            accept(ast->expression);
        }
        return false;
    }

    virtual bool visit(WithStatement *ast)
    {
        out(ast->withToken);
        out(" ");
        out(ast->lparenToken);
        accept(ast->expression);
        out(ast->rparenToken);
        acceptBlockOrIndented(ast->statement);
        return false;
    }

    virtual bool visit(SwitchStatement *ast)
    {
        out(ast->switchToken);
        out(" ");
        out(ast->lparenToken);
        accept(ast->expression);
        out(ast->rparenToken);
        out(" ");
        accept(ast->block);
        return false;
    }

    virtual bool visit(CaseBlock *ast)
    {
        out(ast->lbraceToken);
        newLine();
        accept(ast->clauses);
        if (ast->clauses && ast->defaultClause)
            newLine();
        accept(ast->defaultClause);
        if (ast->moreClauses)
            newLine();
        accept(ast->moreClauses);
        newLine();
        out(ast->rbraceToken);
        return false;
    }

    virtual bool visit(CaseClause *ast)
    {
        out("case ", ast->caseToken);
        accept(ast->expression);
        out(ast->colonToken);
        lnAcceptIndented(ast->statements);
        return false;
    }

    virtual bool visit(DefaultClause *ast)
    {
        out(ast->defaultToken);
        out(ast->colonToken);
        lnAcceptIndented(ast->statements);
        return false;
    }

    virtual bool visit(LabelledStatement *ast)
    {
        out(ast->identifierToken);
        out(": ", ast->colonToken);
        accept(ast->statement);
        return false;
    }

    virtual bool visit(TryStatement *ast)
    {
        out("try ", ast->tryToken);
        accept(ast->statement);
        if (ast->catchExpression) {
            out(" ");
            accept(ast->catchExpression);
        }
        if (ast->finallyExpression) {
            out(" ");
            accept(ast->finallyExpression);
        }
        return false;
    }

    virtual bool visit(Catch *ast)
    {
        out(ast->catchToken);
        out(" ");
        out(ast->lparenToken);
        out(ast->identifierToken);
        out(") ", ast->rparenToken);
        accept(ast->statement);
        return false;
    }

    virtual bool visit(Finally *ast)
    {
        out("finally ", ast->finallyToken);
        accept(ast->statement);
        return false;
    }

    virtual bool visit(FunctionDeclaration *ast)
    {
        return visit(static_cast<FunctionExpression *>(ast));
    }

    virtual bool visit(FunctionExpression *ast)
    {
        out("function ", ast->functionToken);
        if (!ast->name.isNull())
            out(ast->identifierToken);
        out(ast->lparenToken);
        accept(ast->formals);
        out(ast->rparenToken);
        out(" ");
        out(ast->lbraceToken);
        if (ast->body) {
            lnAcceptIndented(ast->body);
            newLine();
        }
        out(ast->rbraceToken);
        return false;
    }


    virtual bool visit(UiImportList *ast)
    {
        for (UiImportList *it = ast; it; it = it->next) {
            accept(it->import);
            newLine();
        }
        requireEmptyLine();
        return false;
    }

    virtual bool visit(UiObjectMemberList *ast)
    {
        for (UiObjectMemberList *it = ast; it; it = it->next) {
            accept(it->member);
            if (it->next)
                newLine();
        }
        return false;
    }

    virtual bool visit(UiArrayMemberList *ast)
    {
        for (UiArrayMemberList *it = ast; it; it = it->next) {
            accept(it->member);
            if (it->next) {
                out(",", ast->commaToken); // ### comma token seems to be generally invalid
                newLine();
            }
        }
        return false;
    }

    virtual bool visit(UiQualifiedId *ast)
    {
        for (UiQualifiedId *it = ast; it; it = it->next) {
            out(it->identifierToken);
            if (it->next)
                out(".");
        }
        return false;
    }

    virtual bool visit(Elision *ast)
    {
        for (Elision *it = ast; it; it = it->next) {
            if (it->next)
                out(", ", ast->commaToken);
        }
        return false;
    }

    virtual bool visit(ArgumentList *ast)
    {
        for (ArgumentList *it = ast; it; it = it->next) {
            accept(it->expression);
            if (it->next) {
                out(", ", it->commaToken);
                addPossibleSplit(-1);
            }
        }
        return false;
    }

    virtual bool visit(StatementList *ast)
    {
        for (StatementList *it = ast; it; it = it->next) {
            // ### work around parser bug: skip empty statements with wrong tokens
            if (EmptyStatement *emptyStatement = cast<EmptyStatement *>(it->statement)) {
                if (toString(emptyStatement->semicolonToken) != QLatin1String(";"))
                    continue;
            }

            accept(it->statement);
            if (it->next)
                newLine();
        }
        return false;
    }

    virtual bool visit(SourceElements *ast)
    {
        for (SourceElements *it = ast; it; it = it->next) {
            accept(it->element);
            if (it->next)
                newLine();
        }
        return false;
    }

    virtual bool visit(VariableDeclarationList *ast)
    {
        for (VariableDeclarationList *it = ast; it; it = it->next) {
            accept(it->declaration);
            if (it->next)
                out(", ", it->commaToken);
        }
        return false;
    }

    virtual bool visit(CaseClauses *ast)
    {
        for (CaseClauses *it = ast; it; it = it->next) {
            accept(it->clause);
            if (it->next)
                newLine();
        }
        return false;
    }

    virtual bool visit(FormalParameterList *ast)
    {
        for (FormalParameterList *it = ast; it; it = it->next) {
            if (it->commaToken.isValid())
                out(", ", it->commaToken);
            out(it->identifierToken);
        }
        return false;
    }
};

} // anonymous namespace

QString QmlJS::reformat(const Document::Ptr &doc)
{
    Rewriter rewriter(doc);
    return rewriter(doc->ast());
}
