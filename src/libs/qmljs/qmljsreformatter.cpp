/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmljsreformatter.h"

#include "qmljscodeformatter.h"
#include "parser/qmljsast_p.h"
#include "parser/qmljsastvisitor_p.h"
#include "parser/qmljsengine_p.h"
#include "parser/qmljslexer_p.h"

#include <QString>
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

    void saveBlockData(QTextBlock *block, const BlockData &data) const override
    {
        block->setUserData(new FormatterData(data));
    }

    bool loadBlockData(const QTextBlock &block, BlockData *data) const override
    {
        if (!block.userData())
            return false;

        *data = static_cast<FormatterData *>(block.userData())->data;
        return true;
    }

    void saveLexerState(QTextBlock *block, int state) const override
    {
        block->setUserState(state);
    }

    int loadLexerState(const QTextBlock &block) const override
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
    int _indent = 0;
    int _nextComment = 0;
    int _lastNewlineOffset = -1;
    bool _hadEmptyLine = false;
    int _binaryExpDepth = 0;

public:
    Rewriter(Document::Ptr doc)
        : _doc(doc)
    {
    }

    void setIndentSize(int size) { _formatter.setIndentSize(size); }
    void setTabSize(int size) { _formatter.setTabSize(size); }

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


        // emit directives
        const QList<SourceLocation> &directives = _doc->jsDirectives();
        for (const auto &d: directives) {
            quint32 line = 1;
            int i = 0;
            while (line++ < d.startLine && i++ >= 0)
                i = _doc->source().indexOf(QChar('\n'), i);
            quint32 offset = static_cast<quint32>(i) + d.startColumn;
            int endline = _doc->source().indexOf('\n', static_cast<int>(offset) + 1);
            int end = endline == -1 ? _doc->source().length() : endline;
            quint32 length =  static_cast<quint32>(end) - offset;
            out(SourceLocation(offset, length, d.startLine, d.startColumn));
        }
        if (!directives.isEmpty())
            newLine();

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

    void lnAcceptIndented(Node *node)
    {
        newLine();
        accept(node);
    }

    void out(const char *str, const SourceLocation &lastLoc = SourceLocation())
    {
        out(QString::fromLatin1(str), lastLoc);
    }

    void outCommentText(const QString &str)
    {
        QStringList lines = str.split(QLatin1Char('\n'));
        bool multiline = lines.length() > 1;
        for (int i = 0; i < lines.size(); ++i) {
            if (multiline) {
                if (i == 0)
                    newLine();
                _line = lines.at(i);  // multiline comments don't keep track of previos lines
            }
            else
                _line += lines.at(i);
            if (i != lines.size() - 1)
                newLine();
        }
        _hadEmptyLine = false;
    }

    void outComment(const SourceLocation &commentLoc)
    {
        SourceLocation fixedLoc = commentLoc;
        fixCommentLocation(fixedLoc);
        if (precededByEmptyLine(fixedLoc) && !_result.endsWith(QLatin1String("\n\n")))
            newLine();
        outCommentText(toString(fixedLoc)); // don't use the sourceloc overload here
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
                if (line.size() > strongMaxLineLength) {
                    result += 50 + (line.size() - strongMaxLineLength);
                // having long lines is bad
                } else if (line.size() > maxLineLength) {
                    result += 3 + (line.size() - maxLineLength);
                // and even ok-sized lines should have a cost
                } else {
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

        while (possibleSplits.count() > 12) {
             QList<Split> newPossibleSplits;
             for (int i = 0; i < possibleSplits.count(); i++) {
                 if (!(i % 2))
                     newPossibleSplits.push_back(possibleSplits.at(i));
             }
             possibleSplits = newPossibleSplits;
        }

        result.badnessFromSplits = 0;
        result.lines = QStringList(line);

        //qCDebug(qmljsLog) << "trying to split" << line << possibleSplits << context;

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
            QString indentLine = newContext.join(QLatin1Char('\n')) + QLatin1String("\n ") + restLine;
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

    bool preVisit(Node *ast) override
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

    void postVisit(Node *ast) override
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

    bool visit(UiPragma *ast) override
    {
        out("pragma ", ast->pragmaToken);
        accept(ast->pragmaType);
        return false;
    }

    bool visit(UiImport *ast) override
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

    bool visit(UiObjectDefinition *ast) override
    {
        accept(ast->qualifiedTypeNameId);
        out(" ");
        accept(ast->initializer);
        return false;
    }

    bool visit(UiObjectInitializer *ast) override
    {
        out(ast->lbraceToken);
        if (ast->members)
            lnAcceptIndented(ast->members);
        newLine();
        out(ast->rbraceToken);
        return false;
    }

    bool visit(UiParameterList *list) override
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

    bool visit(UiPublicMember *ast) override
    {
        if (ast->type == UiPublicMember::Property) {
            if (ast->isDefaultMember)
                out("default ", ast->defaultToken);
            else if (ast->isReadonlyMember)
                out("readonly ", ast->readonlyToken);
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
            if (ast->statement) {
                out(ast->identifierToken);
                out(": ", ast->colonToken);
                accept(ast->statement);
            } else if (ast->binding) {
                accept(ast->binding);
            } else {
                out(ast->identifierToken);
            }
        } else { // signal
            out("signal ", ast->identifierToken);
            out(ast->identifierToken);
            if (ast->parameters) {
                out("(");
                accept(ast->parameters);
                out(")");
            }
        }
        return false;
    }

    bool visit(UiObjectBinding *ast) override
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

    bool visit(UiScriptBinding *ast) override
    {
        accept(ast->qualifiedId);
        out(": ", ast->colonToken);
        accept(ast->statement);
        return false;
    }

    bool visit(UiArrayBinding *ast) override
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

    bool visit(ThisExpression *ast) override { out(ast->thisToken); return true; }
    bool visit(NullExpression *ast) override { out(ast->nullToken); return true; }
    bool visit(TrueLiteral *ast) override { out(ast->trueToken); return true; }
    bool visit(FalseLiteral *ast) override { out(ast->falseToken); return true; }
    bool visit(IdentifierExpression *ast) override { out(ast->identifierToken); return true; }
    bool visit(StringLiteral *ast) override { out(ast->literalToken); return true; }
    bool visit(NumericLiteral *ast) override { out(ast->literalToken); return true; }
    bool visit(RegExpLiteral *ast) override { out(ast->literalToken); return true; }

    bool visit(ArrayLiteral *ast) override
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

    bool visit(ObjectLiteral *ast) override
    {
        out(ast->lbraceToken);
        lnAcceptIndented(ast->properties);
        newLine();
        out(ast->rbraceToken);
        return false;
    }

    bool visit(ElementList *ast) override
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

    bool visit(PropertyAssignmentList *ast) override
    {
        for (PropertyAssignmentList *it = ast; it; it = it->next) {
            PropertyNameAndValue *assignment = AST::cast<PropertyNameAndValue *>(it->assignment);
            if (assignment) {
                out("\"");
                accept(assignment->name);
                out("\"");
                out(": ", assignment->colonToken);
                accept(assignment->value);
                if (it->next) {
                    out(",", ast->commaToken); // always invalid?
                    newLine();
                }
                continue;
            }
            PropertyGetterSetter *getterSetter = AST::cast<PropertyGetterSetter *>(it->assignment);
            if (getterSetter) {
                switch (getterSetter->type) {
                case PropertyGetterSetter::Getter:
                    out("get");
                    break;
                case PropertyGetterSetter::Setter:
                    out("set");
                    break;
                }

                accept(getterSetter->name);
                out("(", getterSetter->lparenToken);
                accept(getterSetter->formals);
                out("(", getterSetter->rparenToken);
                out(" {", getterSetter->lbraceToken);
                accept(getterSetter->functionBody);
                out(" }", getterSetter->rbraceToken);
            }
        }
        return false;
    }

    bool visit(NestedExpression *ast) override
    {
        out(ast->lparenToken);
        accept(ast->expression);
        out(ast->rparenToken);
        return false;
    }

    bool visit(IdentifierPropertyName *ast) override { out(ast->id.toString()); return true; }
    bool visit(StringLiteralPropertyName *ast) override { out(ast->id.toString()); return true; }
    bool visit(NumericLiteralPropertyName *ast) override { out(QString::number(ast->id)); return true; }

    bool visit(ArrayMemberExpression *ast) override
    {
        accept(ast->base);
        out(ast->lbracketToken);
        accept(ast->expression);
        out(ast->rbracketToken);
        return false;
    }

    bool visit(FieldMemberExpression *ast) override
    {
        accept(ast->base);
        out(ast->dotToken);
        out(ast->identifierToken);
        return false;
    }

    bool visit(NewMemberExpression *ast) override
    {
        out("new ", ast->newToken);
        accept(ast->base);
        out(ast->lparenToken);
        accept(ast->arguments);
        out(ast->rparenToken);
        return false;
    }

    bool visit(NewExpression *ast) override
    {
        out("new ", ast->newToken);
        accept(ast->expression);
        return false;
    }

    bool visit(CallExpression *ast) override
    {
        accept(ast->base);
        out(ast->lparenToken);
        addPossibleSplit(0);
        accept(ast->arguments);
        out(ast->rparenToken);
        return false;
    }

    bool visit(PostIncrementExpression *ast) override
    {
        accept(ast->base);
        out(ast->incrementToken);
        return false;
    }

    bool visit(PostDecrementExpression *ast) override
    {
        accept(ast->base);
        out(ast->decrementToken);
        return false;
    }

    bool visit(PreIncrementExpression *ast) override
    {
        out(ast->incrementToken);
        accept(ast->expression);
        return false;
    }

    bool visit(PreDecrementExpression *ast) override
    {
        out(ast->decrementToken);
        accept(ast->expression);
        return false;
    }

    bool visit(DeleteExpression *ast) override
    {
        out("delete ", ast->deleteToken);
        accept(ast->expression);
        return false;
    }

    bool visit(VoidExpression *ast) override
    {
        out("void ", ast->voidToken);
        accept(ast->expression);
        return false;
    }

    bool visit(TypeOfExpression *ast) override
    {
        out("typeof ", ast->typeofToken);
        accept(ast->expression);
        return false;
    }

    bool visit(UnaryPlusExpression *ast) override
    {
        out(ast->plusToken);
        accept(ast->expression);
        return false;
    }

    bool visit(UnaryMinusExpression *ast) override
    {
        out(ast->minusToken);
        accept(ast->expression);
        return false;
    }

    bool visit(TildeExpression *ast) override
    {
        out(ast->tildeToken);
        accept(ast->expression);
        return false;
    }

    bool visit(NotExpression *ast) override
    {
        out(ast->notToken);
        accept(ast->expression);
        return false;
    }

    bool visit(BinaryExpression *ast) override
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

    bool visit(ConditionalExpression *ast) override
    {
        accept(ast->expression);
        out(" ? ", ast->questionToken);
        accept(ast->ok);
        out(" : ", ast->colonToken);
        accept(ast->ko);
        return false;
    }

    bool visit(Block *ast) override
    {
        out(ast->lbraceToken);
        lnAcceptIndented(ast->statements);
        newLine();
        out(ast->rbraceToken);
        return false;
    }

    bool visit(VariableStatement *ast) override
    {
        out("var ", ast->declarationKindToken);
        accept(ast->declarations);
        return false;
    }

    bool visit(VariableDeclaration *ast) override
    {
        out(ast->identifierToken);
        if (ast->expression) {
            out(" = ");
            accept(ast->expression);
        }
        return false;
    }

    bool visit(EmptyStatement *ast) override
    {
        out(ast->semicolonToken);
        return false;
    }

    bool visit(IfStatement *ast) override
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

    bool visit(DoWhileStatement *ast) override
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

    bool visit(WhileStatement *ast) override
    {
        out(ast->whileToken);
        out(" ");
        out(ast->lparenToken);
        accept(ast->expression);
        out(ast->rparenToken);
        acceptBlockOrIndented(ast->statement);
        return false;
    }

    bool visit(ForStatement *ast) override
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

    bool visit(LocalForStatement *ast) override
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

    bool visit(ForEachStatement *ast) override
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

    bool visit(LocalForEachStatement *ast) override
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

    bool visit(ContinueStatement *ast) override
    {
        out(ast->continueToken);
        if (!ast->label.isNull()) {
            out(" ");
            out(ast->identifierToken);
        }
        return false;
    }

    bool visit(BreakStatement *ast) override
    {
        out(ast->breakToken);
        if (!ast->label.isNull()) {
            out(" ");
            out(ast->identifierToken);
        }
        return false;
    }

    bool visit(ReturnStatement *ast) override
    {
        out(ast->returnToken);
        if (ast->expression) {
            out(" ");
            accept(ast->expression);
        }
        return false;
    }

    bool visit(ThrowStatement *ast) override
    {
        out(ast->throwToken);
        if (ast->expression) {
            out(" ");
            accept(ast->expression);
        }
        return false;
    }

    bool visit(WithStatement *ast) override
    {
        out(ast->withToken);
        out(" ");
        out(ast->lparenToken);
        accept(ast->expression);
        out(ast->rparenToken);
        acceptBlockOrIndented(ast->statement);
        return false;
    }

    bool visit(SwitchStatement *ast) override
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

    bool visit(CaseBlock *ast) override
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

    bool visit(CaseClause *ast) override
    {
        out("case ", ast->caseToken);
        accept(ast->expression);
        out(ast->colonToken);
        if (ast->statements)
            lnAcceptIndented(ast->statements);
        return false;
    }

    bool visit(DefaultClause *ast) override
    {
        out(ast->defaultToken);
        out(ast->colonToken);
        lnAcceptIndented(ast->statements);
        return false;
    }

    bool visit(LabelledStatement *ast) override
    {
        out(ast->identifierToken);
        out(": ", ast->colonToken);
        accept(ast->statement);
        return false;
    }

    bool visit(TryStatement *ast) override
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

    bool visit(Catch *ast) override
    {
        out(ast->catchToken);
        out(" ");
        out(ast->lparenToken);
        out(ast->identifierToken);
        out(") ", ast->rparenToken);
        accept(ast->statement);
        return false;
    }

    bool visit(Finally *ast) override
    {
        out("finally ", ast->finallyToken);
        accept(ast->statement);
        return false;
    }

    bool visit(FunctionDeclaration *ast) override
    {
        return visit(static_cast<FunctionExpression *>(ast));
    }

    bool visit(FunctionExpression *ast) override
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


    bool visit(UiHeaderItemList *ast) override
    {
        for (UiHeaderItemList *it = ast; it; it = it->next) {
            accept(it->headerItem);
            newLine();
        }
        requireEmptyLine();
        return false;
    }

    bool visit(UiObjectMemberList *ast) override
    {
        for (UiObjectMemberList *it = ast; it; it = it->next) {
            accept(it->member);
            if (it->next)
                newLine();
        }
        return false;
    }

    bool visit(UiArrayMemberList *ast) override
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

    bool visit(UiQualifiedId *ast) override
    {
        for (UiQualifiedId *it = ast; it; it = it->next) {
            out(it->identifierToken);
            if (it->next)
                out(".");
        }
        return false;
    }

    bool visit(UiQualifiedPragmaId *ast) override
    {
        out(ast->identifierToken);
        return false;
    }

    bool visit(Elision *ast) override
    {
        for (Elision *it = ast; it; it = it->next) {
            if (it->next)
                out(", ", ast->commaToken);
        }
        return false;
    }

    bool visit(ArgumentList *ast) override
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

    bool visit(StatementList *ast) override
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

    bool visit(SourceElements *ast) override
    {
        for (SourceElements *it = ast; it; it = it->next) {
            accept(it->element);
            if (it->next)
                newLine();
        }
        return false;
    }

    bool visit(VariableDeclarationList *ast) override
    {
        for (VariableDeclarationList *it = ast; it; it = it->next) {
            accept(it->declaration);
            if (it->next)
                out(", ", it->commaToken);
        }
        return false;
    }

    bool visit(CaseClauses *ast) override
    {
        for (CaseClauses *it = ast; it; it = it->next) {
            accept(it->clause);
            if (it->next)
                newLine();
        }
        return false;
    }

    bool visit(FormalParameterList *ast) override
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

QString QmlJS::reformat(const Document::Ptr &doc, int indentSize, int tabSize)
{
    Rewriter rewriter(doc);
    rewriter.setIndentSize(indentSize);
    rewriter.setTabSize(tabSize);
    return rewriter(doc->ast());
}
