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

#include <AST.h>
#include <ASTVisitor.h>
#include <Control.h>
#include <Scope.h>
#include <Semantic.h>
#include <TranslationUnit.h>
#include <Literals.h>
#include <Symbols.h>
#include <Names.h>
#include <CoreTypes.h>

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
#include <sstream>

using namespace CPlusPlus;

class CloneCG: protected ASTVisitor
{
public:
    CloneCG(Control *control)
        : ASTVisitor(control)
    { }

    void operator()(AST *ast)
    {
        std::cout <<
            "/**************************************************************************\n"
            "**\n"
            "** This file is part of Qt Creator\n"
            "**\n"
            "** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).\n"
            "**\n"
            "** Contact: Nokia Corporation (qt-info@nokia.com)\n"
            "**\n"
            "** Commercial Usage\n"
            "**\n"
            "** Licensees holding valid Qt Commercial licenses may use this file in\n"
            "** accordance with the Qt Commercial License Agreement provided with the\n"
            "** Software or, alternatively, in accordance with the terms contained in\n"
            "** a written agreement between you and Nokia.\n"
            "**\n"
            "** GNU Lesser General Public License Usage\n"
            "**\n"
            "** Alternatively, this file may be used under the terms of the GNU Lesser\n"
            "** General Public License version 2.1 as published by the Free Software\n"
            "** Foundation and appearing in the file LICENSE.LGPL included in the\n"
            "** packaging of this file.  Please review the following information to\n"
            "** ensure the GNU Lesser General Public License version 2.1 requirements\n"
            "** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.\n"
            "**\n"
            "** If you are unsure which license is appropriate for your use, please\n"
            "** contact the sales department at http://qt.nokia.com/contact.\n"
            "**\n"
            "**************************************************************************/\n"
            "\n"
            "#include \"AST.h\"\n"
            "#include \"ASTVisitor.h\"\n"
            "\n"
            "using namespace CPlusPlus;\n" << std::endl;

        accept(ast);
    }

protected:
    using ASTVisitor::visit;

    QMap<QByteArray, ClassSpecifierAST *> classMap;

    QByteArray id_cast(NameAST *name)
    {
        if (! name)
            return QByteArray();

        Identifier *id = identifier(name->asSimpleName()->identifier_token);

        return QByteArray::fromRawData(id->chars(), id->size());
    }

    void copyMembers(Class *klass)
    {
        for (unsigned i = 0; i < klass->baseClassCount(); ++i) {
            const QByteArray baseClassName = klass->baseClassAt(i)->identifier()->chars();
            if (ClassSpecifierAST *baseClassSpec = classMap.value(baseClassName, 0)) {
                copyMembers(baseClassSpec->symbol);
            }
        }

        const QByteArray className = klass->name()->identifier()->chars();

        std::cout << "    // copy " << className.constData() << std::endl;
        for (unsigned i = 0; i < klass->memberCount(); ++i) {
            Symbol *member = klass->memberAt(i);
            Identifier *id = member->name()->identifier();

            if (! id)
                continue;

            const QByteArray memberName = QByteArray::fromRawData(id->chars(), id->size());
            if (member->type().isUnsigned() && memberName.endsWith("_token")) {
                std::cout << "    ast->" << id->chars() << " = " << id->chars() << ";" << std::endl;
            } else if (PointerType *ptrTy = member->type()->asPointerType()) {
                if (NamedType *namedTy = ptrTy->elementType()->asNamedType()) {
                    QByteArray typeName = namedTy->name()->identifier()->chars();
                    if (typeName.endsWith("AST")) {
                        std::cout << "    if (" << memberName.constData() << ") ast->" << memberName.constData()
                                << " = " << memberName.constData() << "->clone(pool);" << std::endl;
                    }
                }
            }
        }
    }

    virtual bool visit(ClassSpecifierAST *ast)
    {
        Class *klass = ast->symbol;

        const QByteArray className = id_cast(ast->name);
        classMap.insert(className, ast);

        Identifier *clone_id = control()->findOrInsertIdentifier("clone");
        if (! klass->members()->lookat(clone_id))
            return true;

        std::cout << className.constData() << " *" << className.constData() << "::clone(MemoryPool *pool) const" << std::endl
                << "{" << std::endl;

        std::cout << "    " << className.constData() << " *ast = new (pool) " << className.constData() << ";" << std::endl;

        copyMembers(klass);

        std::cout << "    return ast;" << std::endl
                << "}" << std::endl
                << std::endl;

        return true;
    }
};

class SearchListNodes: protected ASTVisitor
{
    QList<QByteArray> _listNodes;

public:
    SearchListNodes(Control *control)
        : ASTVisitor(control)
    { }

    QList<QByteArray> operator()(AST *ast)
    {
        _listNodes.clear();
        accept(ast);
        return _listNodes;
    }

protected:
    virtual bool visit(ClassSpecifierAST *ast)
    {
        for (unsigned i = 0; i < ast->symbol->memberCount(); ++i) {
            Symbol *member = ast->symbol->memberAt(i);
            if (! qstrcmp("next", member->name()->identifier()->chars())) {
                _listNodes.append(ast->symbol->name()->identifier()->chars());
                break;
            }
        }
        return true;
    }
};

class VisitCG: protected ASTVisitor
{
    QList<QByteArray> _listNodes;

public:
    VisitCG(Control *control)
        : ASTVisitor(control)
    { }

    void operator()(AST *ast)
    {
        std::cout <<
            "/**************************************************************************\n"
            "**\n"
            "** This file is part of Qt Creator\n"
            "**\n"
            "** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).\n"
            "**\n"
            "** Contact: Nokia Corporation (qt-info@nokia.com)\n"
            "**\n"
            "** Commercial Usage\n"
            "**\n"
            "** Licensees holding valid Qt Commercial licenses may use this file in\n"
            "** accordance with the Qt Commercial License Agreement provided with the\n"
            "** Software or, alternatively, in accordance with the terms contained in\n"
            "** a written agreement between you and Nokia.\n"
            "**\n"
            "** GNU Lesser General Public License Usage\n"
            "**\n"
            "** Alternatively, this file may be used under the terms of the GNU Lesser\n"
            "** General Public License version 2.1 as published by the Free Software\n"
            "** Foundation and appearing in the file LICENSE.LGPL included in the\n"
            "** packaging of this file.  Please review the following information to\n"
            "** ensure the GNU Lesser General Public License version 2.1 requirements\n"
            "** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.\n"
            "**\n"
            "** If you are unsure which license is appropriate for your use, please\n"
            "** contact the sales department at http://qt.nokia.com/contact.\n"
            "**\n"
            "**************************************************************************/\n"
            "\n"
            "#include \"AST.h\"\n"
            "\n"
            "using namespace CPlusPlus;\n" << std::endl;

        SearchListNodes listNodes(control());
        _listNodes = listNodes(ast);

        accept(ast);
    }

protected:
    using ASTVisitor::visit;

    QMap<QByteArray, ClassSpecifierAST *> classMap;

    QByteArray id_cast(NameAST *name)
    {
        if (! name)
            return QByteArray();

        Identifier *id = identifier(name->asSimpleName()->identifier_token);

        return QByteArray::fromRawData(id->chars(), id->size());
    }

    void visitMembers(Class *klass)
    {
        const QByteArray className = klass->name()->identifier()->chars();

        std::cout << "        // visit " << className.constData() << std::endl;
        for (unsigned i = 0; i < klass->memberCount(); ++i) {
            Symbol *member = klass->memberAt(i);
            Identifier *id = member->name()->identifier();

            if (! id)
                continue;

            const QByteArray memberName = QByteArray::fromRawData(id->chars(), id->size());
            if (member->type().isUnsigned() && memberName.endsWith("_token")) {
                // nothing to do. The member is a token.
            } else if (PointerType *ptrTy = member->type()->asPointerType()) {
                if (NamedType *namedTy = ptrTy->elementType()->asNamedType()) {
                    QByteArray typeName = namedTy->name()->identifier()->chars();
                    if (_listNodes.contains(typeName) && memberName != "next") {
                        std::cout
                                << "        for (" << typeName.constData() << " *it = " << memberName.constData() << "; it; it = it->next)" << std::endl
                                << "            accept(it, visitor);" << std::endl;
                    } else if (typeName.endsWith("AST") && memberName != "next") {
                        std::cout << "        accept(" << memberName.constData() << ", visitor);" << std::endl;
                    }
                }
            }
        }

        for (unsigned i = 0; i < klass->baseClassCount(); ++i) {
            const QByteArray baseClassName = klass->baseClassAt(i)->identifier()->chars();
            if (ClassSpecifierAST *baseClassSpec = classMap.value(baseClassName, 0)) {
                visitMembers(baseClassSpec->symbol);
            }
        }
    }

    virtual bool visit(ClassSpecifierAST *ast)
    {
        Class *klass = ast->symbol;

        const QByteArray className = id_cast(ast->name);
        classMap.insert(className, ast);

        Identifier *visit_id = control()->findOrInsertIdentifier("accept0");
        if (! klass->members()->lookat(visit_id))
            return true;

        std::cout
                << "void " << className.constData() << "::accept0(ASTVisitor *visitor)" << std::endl
                << "{" << std::endl
                << "    if (visitor->visit(this)) {" << std::endl;

        visitMembers(klass);

        std::cout
                << "    }" << std::endl
                << "    visitor->endVisit(this);" << std::endl
                << "}" << std::endl
                << std::endl;

        return true;
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QStringList args = app.arguments();
    const QString appName = args.first();
    args.removeFirst();

    foreach (const QString &arg, args) {
        if (arg == QLatin1String("--help")) {
            const QFileInfo appInfo(appName);
            const QByteArray appFileName = QFile::encodeName(appInfo.fileName());

            printf("Usage: %s [options]\n"
                   "  --help                    Display this information\n",
                   appFileName.constData());

            return EXIT_SUCCESS;
        }
    }

    QFile in;
    if (! in.open(stdin, QFile::ReadOnly))
        return EXIT_FAILURE;

    const QByteArray source = in.readAll();

    Control control;
    StringLiteral *fileId = control.findOrInsertStringLiteral("<stdin>");
    TranslationUnit unit(&control, fileId);
    unit.setObjCEnabled(true);
    unit.setSource(source.constData(), source.size());
    unit.parse();
    if (! unit.ast())
        return EXIT_FAILURE;

    TranslationUnitAST *ast = unit.ast()->asTranslationUnit();
    Q_ASSERT(ast != 0);

    Namespace *globalNamespace = control.newNamespace(0, 0);
    Semantic sem(&control);
    for (DeclarationListAST *it = ast->declarations; it; it = it->next) {
        DeclarationAST *declaration = it->value;
        sem.check(declaration, globalNamespace->members());
    }

    return EXIT_SUCCESS;
}
