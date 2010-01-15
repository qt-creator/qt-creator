/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmlexpressionundercursor.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/parser/qmljsengine_p.h>
#include <qmljs/parser/qmljslexer_p.h>
#include <qmljs/parser/qmljsnodepool_p.h>
#include <qmljs/parser/qmljsparser_p.h>

#include <QDebug>

using namespace Qml;
using namespace QmlJS;
using namespace QmlJS::AST;

namespace QmlEditor {
    namespace Internal {
        class PositionCalculator: protected Visitor
        {
        public:
            Node *operator()(Node *ast, int pos)
            {
                _pos = pos;
                _expression = 0;
                _offset = -1;
                _length = -1;

                Node::accept(ast, this);

                return _expression;
            }

            int offset() const
            { return _offset; }

            int length() const
            { return _length; }

        protected:
            bool visit(FieldMemberExpression *ast)
            {
                if (ast->identifierToken.offset <= _pos && _pos <= ast->identifierToken.end()) {
                    _expression = ast;
                    _offset = ast->identifierToken.offset;
                    _length = ast->identifierToken.length;
                }

                return true;
            }

            bool visit(IdentifierExpression *ast)
            {
                if (ast->firstSourceLocation().offset <= _pos && _pos <= ast->lastSourceLocation().end()) {
                    _expression = ast;
                    _offset = ast->firstSourceLocation().offset;
                    _length = ast->lastSourceLocation().end() - _offset;
                }

                return false;
            }

            bool visit(UiImport * /*ast*/)
            {
                return false;
            }

            bool visit(UiQualifiedId *ast)
            {
                if (ast->identifierToken.offset <= _pos) {
                    for (UiQualifiedId *iter = ast; iter; iter = iter->next) {
                        if (_pos <= iter->identifierToken.end()) {
                            // found it
                            _expression = ast;
                            _offset = ast->identifierToken.offset;

                            for (UiQualifiedId *iter2 = ast; iter2; iter2 = iter2->next) {
                                _length = iter2->identifierToken.end() - _offset;
                            }

                            break;
                        }
                    }
                }

                return false;
            }

        private:
            quint32 _pos;
            Node *_expression;
            int _offset;
            int _length;
        };

        class ScopeCalculator: protected Visitor
        {
        public:
            QStack<QmlSymbol*> operator()(const QmlDocument::Ptr &doc, int pos)
            {
                _doc = doc;
                _pos = pos;
                _scopes.clear();
                _currentSymbol = 0;
                Node::accept(doc->program(), this);
                return _scopes;
            }

        protected:
            virtual bool visit(Block * /*ast*/)
            {
                // TODO
//                if (_pos > ast->lbraceToken.end() && _pos < ast->rbraceToken.offset) {
//                    push(ast);
//                    Node::accept(ast->statements, this);
//                }

                return false;
            }

            virtual bool visit(UiObjectBinding *ast)
            {
                if (ast->initializer && ast->initializer->lbraceToken.offset < _pos && _pos <= ast->initializer->rbraceToken.end()) {
                    push(ast);
                    Node::accept(ast->initializer, this);
                }

                return false;
            }

            virtual bool visit(UiObjectDefinition *ast)
            {
                if (ast->initializer && ast->initializer->lbraceToken.offset < _pos && _pos <= ast->initializer->rbraceToken.end()) {
                    push(ast);
                    Node::accept(ast->initializer, this);
                }

                return false;
            }

            virtual bool visit(UiArrayBinding *ast)
            {
                if (ast->lbracketToken.offset < _pos && _pos <= ast->rbracketToken.end()) {
                    push(ast);
                    Node::accept(ast->members, this);
                }

                return false;
            }

        private:
            void push(Node *node)
            {
                QmlSymbolFromFile* symbol;

                if (_currentSymbol) {
                    symbol = _currentSymbol->findMember(node);
                } else {
                    symbol = _doc->findSymbol(node);
                }

                if (symbol) {
                    _currentSymbol = symbol;

                    if (!cast<UiArrayBinding*>(node))
                        _scopes.push(symbol);
                }
            }

        private:
            QmlDocument::Ptr _doc;
            quint32 _pos;
            QStack<QmlSymbol*> _scopes;
            QmlSymbolFromFile* _currentSymbol;
        };
    }
}

using namespace QmlEditor;
using namespace QmlEditor::Internal;

QmlExpressionUnderCursor::QmlExpressionUnderCursor()
    : _expressionNode(0),
      _pos(0), _engine(0), _nodePool(0)
{
}

QmlExpressionUnderCursor::~QmlExpressionUnderCursor()
{
    if (_engine) { delete _engine; _engine = 0; }
    if (_nodePool) { delete _nodePool; _nodePool = 0; }
}

void QmlExpressionUnderCursor::operator()(const QTextCursor &cursor,
                                          const QmlDocument::Ptr &doc)
{
    if (_engine) { delete _engine; _engine = 0; }
    if (_nodePool) { delete _nodePool; _nodePool = 0; }

    _pos = cursor.position();
    _expressionNode = 0;
    _expressionOffset = -1;
    _expressionLength = -1;
    _expressionScopes.clear();

    const QTextBlock block = cursor.block();
    parseExpression(block);

    if (_expressionOffset != -1) {
        ScopeCalculator calculator;
        _expressionScopes = calculator(doc, _expressionOffset);
    }
}

void QmlExpressionUnderCursor::parseExpression(const QTextBlock &block)
{
    int textPosition = _pos - block.position();
    const QString blockText = block.text();
    if (textPosition > 0) {
        if (blockText.at(textPosition - 1) == QLatin1Char('.'))
            --textPosition;
    } else {
        textPosition = 0;
    }

    const QString text = blockText.left(textPosition);

    Node *node = 0;

    if (UiObjectMember *binding = tryBinding(text)) {
//        qDebug() << "**** binding";
        node = binding;
    } else if (Statement *stmt = tryStatement(text)) {
//        qDebug() << "**** statement";
        node = stmt;
    } else {
//        qDebug() << "**** none";
    }

    if (node) {
        PositionCalculator calculator;
        _expressionNode = calculator(node, textPosition);
        _expressionOffset = calculator.offset() + block.position();
        _expressionLength = calculator.length();
    }
}

Statement *QmlExpressionUnderCursor::tryStatement(const QString &text)
{
    _engine = new Engine();
    _nodePool = new NodePool("", _engine);
    Lexer lexer(_engine);
    Parser parser(_engine);
    lexer.setCode(text, /*line = */ 1);

    if (parser.parseStatement())
        return parser.statement();
    else
        return 0;
}

UiObjectMember *QmlExpressionUnderCursor::tryBinding(const QString &text)
{
    _engine = new Engine();
    _nodePool = new NodePool("", _engine);
    Lexer lexer(_engine);
    Parser parser(_engine);
    lexer.setCode(text, /*line = */ 1);

    if (parser.parseUiObjectMember()) {
        UiObjectMember *member = parser.uiObjectMember();

        if (cast<UiObjectBinding*>(member) || cast<UiArrayBinding*>(member) || cast<UiScriptBinding*>(member))
            return member;
        else
            return 0;
    } else {
        return 0;
    }
}
