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

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>

#include <QFile>
#include <QList>
#include <QCoreApplication>
#include <QStringList>
#include <QFileInfo>
#include <QTime>
#include <QtDebug>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#ifdef __GNUC__
#  include <cxxabi.h>
#endif

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace std;

class ASTDump: protected Visitor
{
public:
    void operator()(const QString &fileName, const QByteArray &src, Node *ast) {
        _src = src;
        QString basename = fileName;
        int dotIdx = basename.lastIndexOf('.');
        if (dotIdx != -1)
            basename.truncate(dotIdx);
        basename.append(QLatin1String(".ast.dot"));
        out.open(basename.toUtf8().constData());

        out << "digraph AST { ordering=out;" << endl;
        // cout << "rankdir = \"LR\";" << endl;
        Node::accept(ast, this);

        typedef QPair<QByteArray, QByteArray> Pair;

        foreach (const Pair &conn, _connections)
            out << conn.first.constData() << " -> " << conn.second.constData() << endl;

        alignTerminals();

        out << "}" << endl;
        out.close();
        cout << qPrintable(basename) << endl;
    }

protected: // visiting methods:
    virtual bool visit(UiImport *ast) {
        terminal(ast->importToken);

        if (ast->importUri)
            nonterminal(ast->importUri);
        else
            terminal(ast->fileNameToken);

        terminal(ast->versionToken);
        terminal(ast->asToken);
        terminal(ast->importIdToken);
        terminal(ast->semicolonToken);
        return false;
    }

    virtual bool visit(UiObjectBinding *ast) {
        if (ast->hasOnToken) {
            nonterminal(ast->qualifiedTypeNameId);
            terminal(ast->colonToken);
            nonterminal(ast->qualifiedId);
        } else {
            nonterminal(ast->qualifiedId);
            terminal(ast->colonToken);
            nonterminal(ast->qualifiedTypeNameId);
        }
        nonterminal(ast->initializer);
        return false;
    }

    virtual bool visit(UiObjectDefinition *ast) {
        nonterminal(ast->qualifiedTypeNameId);
        nonterminal(ast->initializer);
        return false;
    }

    virtual bool visit(UiObjectInitializer *ast) {
        terminal(ast->lbraceToken);
        nonterminal(ast->members);
        terminal(ast->rbraceToken);
        return false;
    }

    virtual bool visit(UiScriptBinding *ast) {
        nonterminal(ast->qualifiedId);
        terminal(ast->colonToken);
        nonterminal(ast->statement);
        return false;
    }

    virtual bool visit(UiArrayBinding *ast) {
        nonterminal(ast->qualifiedId);
        terminal(ast->colonToken);
        terminal(ast->lbracketToken);
        nonterminal(ast->members);
        terminal(ast->rbracketToken);
        return false;
    }

    virtual bool visit(UiArrayMemberList *ast) {
        terminal(ast->commaToken);
        nonterminal(ast->member);
        nonterminal(ast->next);
        return false;
    }

    virtual bool visit(UiQualifiedId *ast) {
        terminal(ast->identifierToken);
        nonterminal(ast->next);
        return false;
    }

    virtual bool visit(UiPublicMember *ast) {
        // TODO: place the paramters...
//        UiParameterList *parameters;

        terminal(ast->defaultToken);
        terminal(ast->readonlyToken);
        terminal(ast->propertyToken);
        terminal(ast->typeModifierToken);
        terminal(ast->typeToken);
        terminal(ast->identifierToken);
        terminal(ast->colonToken);
        nonterminal(ast->expression);
        nonterminal(ast->binding);
        terminal(ast->semicolonToken);
        return false;
    }

    virtual bool visit(StringLiteral *ast) { terminal(ast->literalToken); return false; }
    virtual bool visit(NumericLiteral *ast) { terminal(ast->literalToken); return false; }
    virtual bool visit(TrueLiteral *ast) { terminal(ast->trueToken); return false; }
    virtual bool visit(FalseLiteral *ast) { terminal(ast->falseToken); return false; }
    virtual bool visit(IdentifierExpression *ast) { terminal(ast->identifierToken); return false; }
    virtual bool visit(FieldMemberExpression *ast) { nonterminal(ast->base); terminal(ast->dotToken); terminal(ast->identifierToken); return false; }
    virtual bool visit(BinaryExpression *ast) { nonterminal(ast->left); terminal(ast->operatorToken); nonterminal(ast->right); return false; }
    virtual bool visit(UnaryPlusExpression *ast) { terminal(ast->plusToken); nonterminal(ast->expression); return false; }
    virtual bool visit(UnaryMinusExpression *ast) { terminal(ast->minusToken); nonterminal(ast->expression); return false; }
    virtual bool visit(NestedExpression *ast) { terminal(ast->lparenToken); nonterminal(ast->expression); terminal(ast->rparenToken); return false; }

protected:
    void alignTerminals() {
        out<<"{ rank=same;" << endl;
        foreach (const QByteArray &terminalShape, _terminalShapes) {
            out << "  " << string(terminalShape) << ";" << endl;
        }
        out<<"}"<<endl;
    }

    static QByteArray name(Node *ast) {
#ifdef __GNUC__
        QByteArray name = abi::__cxa_demangle(typeid(*ast).name(), 0, 0, 0) + 12;
#else
        QByteArray name = typeid(*ast).name();
#endif
        return name;
    }

    QString spell(const SourceLocation &token) {
        return _src.mid(token.offset, token.length).replace('\'', "\\\\").replace('"', "\\\"");
    }

    void terminal(const SourceLocation &token) {
        if (!token.isValid())
            return;

        static int count = 1;
        QByteArray id = 't' + QByteArray::number(count++);
        Node *node = _stack.last();
        _connections.append(qMakePair(_id[node], id));

        QByteArray t;
        t.append(id);
        t.append(" [label = \"");
        t.append(spell(token));
        t.append("\" shape=rect]");
        _terminalShapes.append(t);
    }

    virtual void nonterminal(Node *ast) {
        Node::accept(ast, this);
    }

    virtual void node(Node *ast) {
        out << _id[ast].constData() << " [label=\"" << name(ast).constData() << "\"];" << endl;
    }

    virtual bool preVisit(Node *ast) {
        static int count = 1;
        const QByteArray id = 'n' + QByteArray::number(count++);
        _id[ast] = id;


        if (! _stack.isEmpty())
            _connections.append(qMakePair(_id[_stack.last()], id));

        _stack.append(ast);

        node(ast);

        return true;
    }

    virtual void postVisit(Node *) {
        _stack.removeLast();
    }

private:
    QHash<Node *, QByteArray> _id;
    QList<QPair<QByteArray, QByteArray> > _connections;
    QList<Node *> _stack;
    QList<QByteArray> _terminalShapes;
    ofstream out;
    QByteArray _src;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QStringList files = app.arguments();
    files.removeFirst();

    foreach (const QString &fileName, files) {
        QFile file(fileName);
        if (! file.open(QFile::ReadOnly)) {
            cerr << "Cannot open \"" << qPrintable(fileName)
                      << "\", skipping it." << endl;
            continue;
        }

        const QByteArray source = file.readAll();
        file.close();

        Document::Ptr doc = Document::create(fileName);
        doc->setSource(source);
        doc->parseQml();

        foreach (const DiagnosticMessage &m, doc->diagnosticMessages()) {
            ostream *os;
            if (m.isError()) {
                os = &cerr;
                *os << "Error:";
            } else {
                os = &cout;
                *os << "Warning:";
            }

            if (m.loc.isValid())
                *os << m.loc.startLine << ':' << m.loc.startColumn << ':';
            *os << ' ';
            *os << qPrintable(m.message) << endl;
        }

        ASTDump dump;
        dump(fileName, source, doc->qmlProgram());
    }

    return EXIT_SUCCESS;
}
