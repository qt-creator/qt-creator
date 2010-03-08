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

#include <QFile>
#include <QList>
#include <QCoreApplication>
#include <QStringList>
#include <QFileInfo>
#include <QTime>
#include <QtDebug>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cxxabi.h>

using namespace CPlusPlus;

class ASTDump: protected ASTVisitor
{
public:
    ASTDump(TranslationUnit *unit)
        : ASTVisitor(unit) {}

    void operator()(AST *ast) {
        std::cout << "digraph AST {" << std::endl;
        // std::cout << "rankdir = \"LR\";" << std::endl;
        accept(ast);
        std::cout << "}" << std::endl;
    }

protected:
    std::string name(AST *ast) const {
        QByteArray name = abi::__cxa_demangle(typeid(*ast).name(), 0, 0, 0) + 11;
        name.truncate(name.length() - 3);
        name = QByteArray::number(_id.value(ast)) + ". " + name;

        if (ast->lastToken() - ast->firstToken() == 1) {
            const char *x = spell(ast->firstToken());
            name += ' ';
            name += x;
        }

        name.prepend('"');
        name.append('"');        

        return std::string(name);
    }

    virtual bool preVisit(AST *ast) {
        static int count = 1;
        _id[ast] = count++;

        if (! _stack.isEmpty())
            std::cout << name(_stack.last()) << " -> " << name(ast) << ";" << std::endl;

        _stack.append(ast);

        return true;
    }

    virtual void postVisit(AST *) {
        _stack.removeLast();        
    }

private:
    QHash<AST *, int> _id;
    QList<AST *> _stack;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QStringList files = app.arguments();
    files.removeFirst();

    foreach (const QString &fileName, files) {
        QFile file(fileName);
        if (! file.open(QFile::ReadOnly))
            continue;

        const QByteArray source = file.readAll();
        file.close();

        Document::Ptr doc = Document::create(fileName);
        doc->control()->setDiagnosticClient(0);
        doc->setSource(source);
        doc->parse();

        ASTDump dump(doc->translationUnit());
        dump(doc->translationUnit()->ast());
    }

    return EXIT_SUCCESS;
}
