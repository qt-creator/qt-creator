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

#include <AST.h>
#include <ASTVisitor.h>
#include <ASTPatternBuilder.h>
#include <ASTMatcher.h>
#include <Control.h>
#include <Scope.h>
#include <Bind.h>
#include <TranslationUnit.h>
#include <Literals.h>
#include <Symbols.h>
#include <Names.h>
#include <CoreTypes.h>
#include <CppDocument.h>
#include <SymbolVisitor.h>
#include <Overview.h>
#include <LookupContext.h>

#include "cplusplus-tools-utils.h"

#include <QFile>
#include <QList>
#include <QCoreApplication>
#include <QStringList>
#include <QFileInfo>
#include <QTime>
#include <QDebug>

#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace CPlusPlus;

class MkVisitor: protected SymbolVisitor
{
    const LookupContext &context;
    Overview oo;
    QList<ClassOrNamespace *> interfaces;
    QList<ClassOrNamespace *> nodes;

    bool isMiscNode(ClassOrNamespace *b) const
    {
        foreach (ClassOrNamespace *u, b->usings()) {
            if (oo(u->symbols().first()->name()) == QLatin1String("AST"))
                return true;
        }

        return false;
    }

    QString getAcceptFunctionName(ClassOrNamespace *b, QString *retType) const
    {
        Q_ASSERT(b != 0);

        retType->clear();

        if (interfaces.contains(b) || isMiscNode(b)) {
            QString className = oo(b->symbols().first()->name());

            if (className.endsWith(QLatin1String("AST"))) {
                className.chop(3);

                QString funcName = className;
                if (! funcName.isEmpty())
                    funcName[0] = funcName[0].toLower();

                if (funcName == QLatin1String("operator"))
                    funcName = QLatin1String("cppOperator");

                if (interfaces.contains(b))
                    *retType = className + QLatin1String("Ty");

                return funcName;
            }
        }

        if (! b->usings().isEmpty())
            return getAcceptFunctionName(b->usings().first(), retType);

        Q_ASSERT(!"wtf?");
        return QString();
    }

public:
    MkVisitor(const LookupContext &context): context(context) {
        accept(context.thisDocument()->globalNamespace());

        // declaration

        std::cout << "#include <CPlusPlus.h>" << std::endl
                  << "#include <memory>" << std::endl
                  << "#include <cassert>" << std::endl
                  << std::endl
                  << "using namespace CPlusPlus;" << std::endl
                  << std::endl;


        std::cout << "class Semantic: protected ASTVisitor" << std::endl
                  << "{" << std::endl
                  << "public:" << std::endl
                  << "    Semantic(TranslationUnit *unit): ASTVisitor(unit) { translationUnit(unit->ast()->asTranslationUnit()); }" << std::endl
                  << std::endl;

        foreach (ClassOrNamespace *b, interfaces) {
            Q_ASSERT(! b->symbols().isEmpty());

            Class *klass = 0;
            foreach (Symbol *s, b->symbols())
                if ((klass = s->asClass()) != 0)
                    break;

            Q_ASSERT(klass != 0);

            QString className = oo(klass->name());
            if (className == QLatin1String("AST"))
                continue;

            QString baseClassName = className;
            baseClassName.chop(3);

            QString retTy;
            QString funcName = getAcceptFunctionName(b, &retTy);

            std::cout
                    << "    typedef void *" << qPrintable(baseClassName) << "Ty;" << std::endl
                    << "    " << qPrintable(baseClassName) << "Ty " << qPrintable(funcName)
                    << "(" << qPrintable(className) << " *ast);" << std::endl
                    << std::endl;
        }

        std::cout << "protected:" << std::endl;
        std::cout << "    using ASTVisitor::translationUnit;" << std::endl
                  << std::endl;

        QHash<ClassOrNamespace *, QList<ClassOrNamespace *> > implements;
        foreach (ClassOrNamespace *b, nodes) {
            ClassOrNamespace *iface = 0;
            foreach (ClassOrNamespace *u, b->usings()) {
                if (interfaces.contains(u)) {
                    iface = u;
                    break;
                }
            }
            Q_ASSERT(iface != 0);
            implements[iface].append(b);
        }

        foreach (ClassOrNamespace *iface, interfaces) {
            foreach (ClassOrNamespace *b, implements.value(iface)) {
                if (! isMiscNode(b))
                    continue;

                Class *klass = 0;
                foreach (Symbol *s, b->symbols())
                    if ((klass = s->asClass()) != 0)
                        break;

                Q_ASSERT(klass != 0);

                QString retTy ;
                QString className = oo(klass->name());
                std::cout << "    void " << qPrintable(getAcceptFunctionName(b, &retTy)) << "(" << qPrintable(className) << " *ast);" << std::endl;
            }
        }

        std::cout << std::endl;

        foreach (ClassOrNamespace *iface, interfaces) {
            std::cout << "    // " << qPrintable(oo(iface->symbols().first()->name())) << std::endl;
            foreach (ClassOrNamespace *b, implements.value(iface)) {
                Class *klass = 0;
                foreach (Symbol *s, b->symbols())
                    if ((klass = s->asClass()) != 0)
                        break;

                Q_ASSERT(klass != 0);

                QString className = oo(klass->name());
                std::cout << "    virtual bool visit(" << qPrintable(className) << " *ast);" << std::endl;
            }
            std::cout << std::endl;
        }

        std::cout << "private:" << std::endl;
        foreach (ClassOrNamespace *b, interfaces) {
            Q_ASSERT(! b->symbols().isEmpty());

            Class *klass = 0;
            foreach (Symbol *s, b->symbols())
                if ((klass = s->asClass()) != 0)
                    break;

            Q_ASSERT(klass != 0);

            QString className = oo(klass->name());
            if (className == QLatin1String("AST"))
                continue;

            QString baseClassName = className;
            baseClassName.chop(3);

            QString current = QLatin1String("_current");
            current += baseClassName;

            std::cout << "    " << qPrintable(baseClassName) << "Ty " << qPrintable(current) << ";" << std::endl;
        }

        std::cout << "};" << std::endl
                  << std::endl;


        std::cout << "namespace { bool debug_todo = false; }" << std::endl
                  << std::endl;


        // implementation

        foreach (ClassOrNamespace *b, interfaces) {
            Q_ASSERT(! b->symbols().isEmpty());

            Class *klass = 0;
            foreach (Symbol *s, b->symbols())
                if ((klass = s->asClass()) != 0)
                    break;

            Q_ASSERT(klass != 0);

            QString className = oo(klass->name());
            if (className == QLatin1String("AST"))
                continue;

            QString baseClassName = className;
            baseClassName.chop(3);

            QString retTy;
            QString funcName = getAcceptFunctionName(b, &retTy);

            QString current = QLatin1String("_current");
            current += baseClassName;

            std::cout << "Semantic::" << qPrintable(baseClassName) << "Ty Semantic::"
                      << qPrintable(funcName) << "(" << qPrintable(className) << " *ast)" << std::endl
                      << "{" << std::endl
                      << "    " << qPrintable(baseClassName) << "Ty value = " << qPrintable(baseClassName) << "Ty();" << std::endl
                      << "    std::swap(" << qPrintable(current) << ", value);" << std::endl
                      << "    accept(ast);" << std::endl
                      << "    std::swap(" << qPrintable(current) << ", value);" << std::endl
                      << "    return value;" << std::endl
                      << "}" << std::endl
                      << std::endl;
        }

        foreach (ClassOrNamespace *iface, interfaces) {
            std::cout << "// " << qPrintable(oo(iface->symbols().first()->name())) << std::endl;
            foreach (ClassOrNamespace *b, implements.value(iface)) {
                Class *klass = 0;
                foreach (Symbol *s, b->symbols())
                    if ((klass = s->asClass()) != 0)
                        break;

                Q_ASSERT(klass != 0);

                QString className = oo(klass->name());
                std::cout << "bool Semantic::visit(" << qPrintable(className) << " *ast)" << std::endl
                          << "{" << std::endl;

                if (isMiscNode(b)) {
                    std::cout << "    (void) ast;" << std::endl
                              << "    assert(!\"unreachable\");" << std::endl
                              << "    return false;" << std::endl
                              << "}" << std::endl
                              << std::endl;

                    QString retTy;
                    std::cout << "void Semantic::" << qPrintable(getAcceptFunctionName(b, &retTy)) << "(" << qPrintable(className) << " *ast)" << std::endl
                              << "{" << std::endl
                              << "    if (! ast)" << std::endl
                              << "        return;" << std::endl
                              << std::endl;
                }

                std::cout
                        << "    if (debug_todo)" << std::endl
                        << "        translationUnit()->warning(ast->firstToken(), \"TODO: %s\", __func__);" << std::endl;

                for (unsigned i = 0; i < klass->memberCount(); ++i) {
                    Declaration *decl = klass->memberAt(i)->asDeclaration();
                    if (! decl)
                        continue;
                    if (decl->type()->isFunctionType())
                        continue;
                    const QString declName = oo(decl->name());
                    if (PointerType *ptrTy = decl->type()->asPointerType()) {
                        if (NamedType *namedTy = ptrTy->elementType()->asNamedType()) {
                            const QString eltTyName = oo(namedTy->name());
                            if (eltTyName.endsWith(QLatin1String("ListAST"))) {
                                QString name = eltTyName;
                                name.chop(7);
                                name += QLatin1String("AST");

                                Control *control = context.thisDocument()->control();
                                const Name *n = control->identifier(name.toLatin1().constData());

                                if (ClassOrNamespace *bb = context.lookupType(n, klass)) {
                                    QString retTy;
                                    QString funcName = getAcceptFunctionName(bb, &retTy);
                                    Q_ASSERT(! funcName.isEmpty());

                                    QString var;
                                    if (! retTy.isEmpty())
                                        var += retTy + QLatin1String(" value = ");
                                    //std::cout << "    // ### accept with function " << qPrintable(getAcceptFunctionName(bb, &hasRetValue)) << std::endl;
                                    std::cout << "    for (" << qPrintable(eltTyName) << " *it = ast->" << qPrintable(declName) << "; it; it = it->next) {" << std::endl
                                              << "        " << qPrintable(var) << "this->" << qPrintable(funcName) << "(it->value);" << std::endl
                                              << "    }" << std::endl;
                                    continue;
                                }

                                std::cout << "    // ### process list of " << qPrintable(name) << std::endl;
                                continue;
                            }

                            if (ClassOrNamespace *ty = context.lookupType(namedTy->name(), klass)) {
                                QString className = oo(ty->symbols().first()->name());
                                QString baseClassName = className;
                                if (baseClassName.endsWith(QLatin1String("AST"))) {
                                    baseClassName.chop(3);

                                    QString retTy;
                                    QString funcName = getAcceptFunctionName(ty, &retTy);
                                    QString var;
                                    if (! retTy.isEmpty())
                                        var += retTy + QLatin1String(" ") + declName + QLatin1String(" = ");
                                    std::cout << "    " << qPrintable(var) << "this->" << qPrintable(funcName) << "(ast->" << qPrintable(declName) << ");" << std::endl;
                                    continue;
                                }
                            }
                        }
                    }

                    std::cout << "    // " << qPrintable(oo.prettyType(decl->type(), declName)) << " = ast->" << qPrintable(declName) << ";" << std::endl;
                }

                if (! isMiscNode(b))
                    std::cout << "    return false;" << std::endl;

                std::cout
                          << "}" << std::endl
                          << std::endl;
            }
            std::cout << std::endl;
        }

    }

protected:
    using SymbolVisitor::visit;

    QList<ClassOrNamespace *> baseClasses(ClassOrNamespace *b) {
        QList<ClassOrNamespace *> usings = b->usings();
        foreach (ClassOrNamespace *u, usings)
            usings += baseClasses(u);
        return usings;
    }

    virtual bool visit(Class *klass) {
        const QString className = oo(klass->name());
        if (! className.endsWith(QLatin1String("AST")))
            return false;

        ClassOrNamespace *b = context.lookupType(klass);
        Q_ASSERT(b != 0);

        const Identifier *accept0 = context.thisDocument()->control()->identifier("accept0");
        if (Symbol *s = klass->find(accept0)) {
            if (Function *meth = s->type()->asFunctionType()) {
                if (! meth->isPureVirtual()) {
                    foreach (ClassOrNamespace *u, b->usings()) {
                        if (interfaces.contains(u)) {
                            // qDebug() << oo(klass->name()) << "implements" << oo(u->symbols().first()->name());
                        } else {
                            Q_ASSERT(!"no way");
                        }
                    }
                    nodes.append(b);
                    return false;
                }
            }
        }

        //qDebug() << oo(klass->name()) << "is a base ast node";
        interfaces.append(b);

        return false;
    }
};

void printUsage()
{
    std::cout << "Usage: " << qPrintable(QFileInfo(qApp->arguments().at(0)).fileName())
              << " [-v] [path to AST.h]\n\n"
              << "Print a visitor class based on AST.h to stdout.\n\n";
    const QString defaulPath = QFileInfo(QLatin1String(PATH_AST_H)).canonicalFilePath();
    std::cout << "Default path: " << qPrintable(defaulPath) << '.' << "\n";
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    args.removeFirst();

    bool optionVerbose = false;

    // Process options & arguments
    if (args.contains(QLatin1String("-v"))) {
        optionVerbose = true;
        args.removeOne(QLatin1String("-v"));
    }
    const bool helpRequested = args.contains(QLatin1String("-h"))
        || args.contains(QLatin1String("-help"));
    if (helpRequested || args.count() >= 2) {
        printUsage();
        return helpRequested ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    // Run the preprocessor
    QString fileName = QLatin1String(PATH_AST_H);
    if (!args.isEmpty())
        fileName = args.first();

    const QString fileNamePreprocessed = fileName + QLatin1String(".preprocessed");
    CplusplusToolsUtils::SystemPreprocessor preprocessor(optionVerbose);
    preprocessor.preprocessFile(fileName, fileNamePreprocessed);

    QFile file(fileNamePreprocessed);
    if (! file.open(QFile::ReadOnly)) {
        std::cerr << "Error: Could not open file \"" << qPrintable(file.fileName()) << "\"."
                  << std::endl;
        return EXIT_FAILURE;
    }

    const QByteArray source = file.readAll();
    file.close();

    Document::Ptr doc = Document::create(fileName);
    //doc->control()->setDiagnosticClient(0);
    doc->setUtf8Source(source);
    doc->parse();
    doc->translationUnit()->blockErrors(true);
    doc->check();

    Snapshot snapshot;
    snapshot.insert(doc);

    LookupContext context(doc, snapshot);
    MkVisitor mkVisitor(context);

    return EXIT_SUCCESS;
}
