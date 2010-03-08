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

#include <AST.h>
#include <ASTVisitor.h>
#include <ASTPatternBuilder.h>
#include <ASTMatcher.h>
#include <Control.h>
#include <Scope.h>
#include <Semantic.h>
#include <TranslationUnit.h>
#include <Literals.h>
#include <Symbols.h>
#include <Names.h>
#include <CoreTypes.h>
#include <CppDocument.h>
#include <SymbolVisitor.h>

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
#include <cxxabi.h>

using namespace CPlusPlus;

class ASTDump: protected ASTVisitor
{
public:
    ASTDump(TranslationUnit *unit)
        : ASTVisitor(unit) {}

    void operator()(AST *ast) {
        QByteArray basename = translationUnit()->fileName();
        int dotIdx = basename.lastIndexOf('.');
        if (dotIdx != -1)
            basename.truncate(dotIdx);
        basename.append(".ast.dot");
        out.open(basename.constData());

        out << "digraph AST {" << std::endl;
        // std::cout << "rankdir = \"LR\";" << std::endl;
        accept(ast);

        foreach (const QByteArray &terminalShape, _terminalShapes) {
            out << std::string(terminalShape) << " [shape=rect]" << std::endl;
        }

        alignTerminals();

        out << "}" << std::endl;
        out.close();
        std::cout << basename.constData() << std::endl;
    }

protected:
    void alignTerminals() {
        out<<"{ rank=same; ";
        foreach (const QByteArray &terminalShape, _terminalShapes) {
            out << std::string(terminalShape) << "; ";
        }
        out<<"}"<<std::endl;
    }

    QByteArray addTerminalInfo(AST *ast) const {
        if (SimpleNameAST *simpleName = ast->asSimpleName()) {
            return spell(simpleName->identifier_token);
        } else if (BinaryExpressionAST *binExpr = ast->asBinaryExpression()) {
            return spell(binExpr->binary_op_token);
        } else if (SimpleSpecifierAST *simpleSpec = ast->asSimpleSpecifier()) {
            return spell(simpleSpec->specifier_token);
        } else if (NumericLiteralAST *numLit = ast->asNumericLiteral()) {
            return spell(numLit->literal_token);
        } else if (StringLiteralAST *strLit = ast->asStringLiteral()) {
            return spell(strLit->literal_token);
        } else if (BoolLiteralAST *boolLit = ast->asBoolLiteral()) {
            return spell(boolLit->literal_token);
        } else {
            return QByteArray();
        }
    }

    std::string name(AST *ast) {
        QByteArray name = abi::__cxa_demangle(typeid(*ast).name(), 0, 0, 0) + 11;
        name.truncate(name.length() - 3);
        name = QByteArray::number(_id.value(ast)) + ". " + name;

        QByteArray info = addTerminalInfo(ast);
        if (!info.isEmpty()) {
            name.append("\\n");
            name.append(info);
        }
        name.prepend('"');
        name.append('"');

        if (!info.isEmpty())
            _terminalShapes.insert(name);

        return std::string(name);
    }

    virtual bool preVisit(AST *ast) {
        static int count = 1;
        _id[ast] = count++;

        if (! _stack.isEmpty())
            out << name(_stack.last()) << " -> " << name(ast) << ";" << std::endl;

        _stack.append(ast);

        return true;
    }

    virtual void postVisit(AST *) {
        _stack.removeLast();
    }

private:
    QHash<AST *, int> _id;
    QList<AST *> _stack;
    QSet<QByteArray> _terminalShapes;
    std::ofstream out;
};

class SymbolDump: protected SymbolVisitor
{
public:
    SymbolDump(TranslationUnit *unit)
        : translationUnit(unit)
    {}

    void operator()(Symbol *s) {
        QByteArray basename = translationUnit->fileName();
        int dotIdx = basename.lastIndexOf('.');
        if (dotIdx != -1)
            basename.truncate(dotIdx);
        basename.append(".symbols.dot");
        out.open(basename.constData());

        out << "digraph Symbols {" << std::endl;
        // std::cout << "rankdir = \"LR\";" << std::endl;
        accept(s);

        for (int i = 0; i < _connections.size(); ++i) {
            QPair<Symbol*,Symbol*> connection = _connections.at(i);
            QByteArray from = _id.value(connection.first);
            if (from.isEmpty())
                from = name(connection.first);
            QByteArray to = _id.value(connection.second);
            if (to.isEmpty())
                to = name(connection.second);
            out << from.constData() << " -> " << to.constData() << ";" << std::endl;
        }

        out << "}" << std::endl;
        out.close();
        std::cout << basename.constData() << std::endl;
    }

protected:
    QByteArray name(Symbol *s) {
        QByteArray name = abi::__cxa_demangle(typeid(*s).name(), 0, 0, 0) + 11;
        if (s->identifier()) {
            name.append("\\nid: ");
            name.append(s->identifier()->chars());
        }

        return name;
    }

    virtual bool preVisit(Symbol *s) {
        static int count = 0;
        QByteArray nodeId("s");
        nodeId.append(QByteArray::number(++count));
        _id[s] = nodeId;

        if (!_stack.isEmpty())
            _connections.append(qMakePair(_stack.last(), s));

        _stack.append(s);

        return true;
    }

    virtual void postVisit(Symbol *) {
        _stack.removeLast();
    }

    virtual void simpleNode(Symbol *symbol) {
        out << _id[symbol].constData() << " [label=\"" << name(symbol).constData() << "\"];" << std::endl;
    }

    void generateTemplateParams(const char *from, TemplateParameters *params) {
        if (!params)
            return;

        static int templateCount = 0;
        QByteArray id("t");
        id.append(QByteArray::number(++templateCount));
        out << from << " -> " << id.constData() << ";" << std::endl;
        out << id.constData() << " [shape=record label=\"";
        for (unsigned i = 0; i < params->scope()->symbolCount(); ++i) {
            if (i > 0)
                out<<"|";
            Symbol *s = params->scope()->symbolAt(i);
            if (s->identifier()) {
                out<<s->identifier()->chars();
            } else {
                out << "<anonymous>";
            }
        }
        out << "\"];" << std::endl;
    }

    virtual bool visit(Class *symbol) {
        const char *id = _id.value(symbol).constData();
        out << id << " [label=\"";
        if (symbol->isClass()) {
            out << "class";
        } else if (symbol->isStruct()) {
            out << "struct";
        } else if (symbol->isUnion()) {
            out << "union";
        } else {
            out << "UNKNOWN";
        }
        out << "\\nid: ";
        if (symbol->identifier()) {
            out << symbol->identifier()->chars();
        } else {
            out << "NO ID";
        }
        out << "\"];" << std::endl;

        generateTemplateParams(id, symbol->templateParameters());

        return true;
    }

    virtual bool visit(UsingNamespaceDirective *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(UsingDeclaration *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(Declaration *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(Argument *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(TypenameArgument *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(BaseClass *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(Enum *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(Function *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(Namespace *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(Block *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(ForwardClassDeclaration *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(ObjCBaseClass *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(ObjCBaseProtocol *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(ObjCClass *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(ObjCForwardClassDeclaration *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(ObjCProtocol *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(ObjCForwardProtocolDeclaration *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(ObjCMethod *symbol) { simpleNode(symbol); return true; }
    virtual bool visit(ObjCPropertyDeclaration *symbol) { simpleNode(symbol); return true; }

private:
    TranslationUnit *translationUnit;
    QHash<Symbol *, QByteArray> _id;
    QList<QPair<Symbol *,Symbol*> >_connections;
    QList<Symbol *> _stack;
    std::ofstream out;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QStringList files = app.arguments();
    files.removeFirst();

    foreach (const QString &fileName, files) {
        QFile file(fileName);
        if (! file.open(QFile::ReadOnly)) {
            std::cerr << "Cannot open \"" << qPrintable(fileName)
                      << "\", skipping it." << std::endl;
            continue;
        }

        const QByteArray source = file.readAll();
        file.close();

        Document::Ptr doc = Document::create(fileName);
        doc->control()->setDiagnosticClient(0);
        doc->setSource(source);
        doc->parse();

        doc->check();

        ASTDump dump(doc->translationUnit());
        dump(doc->translationUnit()->ast());

        SymbolDump dump2(doc->translationUnit());
        dump2(doc->globalNamespace());
    }

    return EXIT_SUCCESS;
}
