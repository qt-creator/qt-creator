
#include <QCoreApplication>
#include <QStringList>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QDir>
#include <QDebug>

#include <Control.h>
#include <Parser.h>
#include <AST.h>
#include <ASTVisitor.h>
#include <Symbols.h>
#include <CoreTypes.h>
#include <Literals.h>
#include <CppDocument.h>
#include <Overview.h>
#include <Names.h>
#include <Scope.h>

#include <iostream>
#include <cstdlib>

using namespace CPlusPlus;

static const char copyrightHeader[] =
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
;

class ASTNodes
{
public:
    ASTNodes(): base(0) {}

    ClassSpecifierAST *base; // points to "class AST"
    QList<ClassSpecifierAST *> deriveds; // n where n extends AST
    QList<QTextCursor> endOfPublicClassSpecifiers;
};

static ASTNodes astNodes;

static QTextCursor createCursor(TranslationUnit *unit, AST *ast, QTextDocument *document)
{
    unsigned startLine, startColumn, endLine, endColumn;
    unit->getTokenStartPosition(ast->firstToken(), &startLine, &startColumn);
    unit->getTokenEndPosition(ast->lastToken() - 1, &endLine, &endColumn);

    QTextCursor tc(document);
    tc.setPosition(document->findBlockByNumber(startLine - 1).position());
    tc.setPosition(document->findBlockByNumber(endLine - 1).position() + endColumn - 1,
                   QTextCursor::KeepAnchor);

    int charsToSkip = 0;
    forever {
        QChar ch = document->characterAt(tc.position() + charsToSkip);

        if (! ch.isSpace())
            break;

        ++charsToSkip;

        if (ch == QChar::ParagraphSeparator)
            break;
    }

    tc.setPosition(tc.position() + charsToSkip, QTextCursor::KeepAnchor);
    return tc;
}

class FindASTNodes: protected ASTVisitor
{
public:
    FindASTNodes(Document::Ptr doc, QTextDocument *document)
        : ASTVisitor(doc->control()), document(document)
    {
    }

    ASTNodes operator()(AST *ast)
    {
        accept(ast);
        return _nodes;
    }

protected:
    virtual bool visit(ClassSpecifierAST *ast)
    {
        Class *klass = ast->symbol;
        Q_ASSERT(klass != 0);

        const QString className = oo(klass->name());

        if (className.endsWith("AST")) {
            if (className == QLatin1String("AST"))
                _nodes.base = ast;
            else {
                _nodes.deriveds.append(ast);

                AccessDeclarationAST *accessDeclaration = 0;
                for (DeclarationListAST *it = ast->member_specifier_list; it; it = it->next) {
                    if (AccessDeclarationAST *decl = it->value->asAccessDeclaration()) {
                        if (tokenKind(decl->access_specifier_token) == T_PUBLIC)
                            accessDeclaration = decl;
                    }
                }

                if (! accessDeclaration)
                    qDebug() << "no access declaration for class:" << className;

                Q_ASSERT(accessDeclaration != 0);

                QTextCursor tc = createCursor(translationUnit(), accessDeclaration, document);
                tc.setPosition(tc.position());

                _nodes.endOfPublicClassSpecifiers.append(tc);
            }
        }

        return true;
    }

private:
    QTextDocument *document;
    ASTNodes _nodes;
    Overview oo;
};

class Accept0CG: protected ASTVisitor
{
    QDir _cplusplusDir;
    QTextStream *out;

public:
    Accept0CG(const QDir &cplusplusDir, Control *control)
        : ASTVisitor(control), _cplusplusDir(cplusplusDir), out(0)
    { }

    void operator()(AST *ast)
    {
        QFileInfo fileInfo(_cplusplusDir, QLatin1String("ASTVisit.cpp"));

        QFile file(fileInfo.absoluteFilePath());
        if (! file.open(QFile::WriteOnly))
            return;

        QTextStream output(&file);
        out = &output;

        *out << copyrightHeader <<
            "\n"
            "#include \"AST.h\"\n"
            "#include \"ASTVisitor.h\"\n"
            "\n"
            "using namespace CPlusPlus;\n" << endl;

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
        // *out << "        // visit " << className.constData() << endl;
        for (unsigned i = 0; i < klass->memberCount(); ++i) {
            Symbol *member = klass->memberAt(i);
            if (! member->name())
                continue;

            Identifier *id = member->name()->identifier();

            if (! id)
                continue;

            const QByteArray memberName = QByteArray::fromRawData(id->chars(), id->size());
            if (member->type().isUnsigned() && memberName.endsWith("_token")) {
                // nothing to do. The member is a token.

            } else if (PointerType *ptrTy = member->type()->asPointerType()) {

                if (NamedType *namedTy = ptrTy->elementType()->asNamedType()) {
                    QByteArray typeName = namedTy->name()->identifier()->chars();

                    if (typeName.endsWith("AST") && memberName != "next") {
                        *out << "        accept(" << memberName.constData() << ", visitor);" << endl;
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

    bool checkMethod(Symbol *accept0Method) const
    {
        Declaration *decl = accept0Method->asDeclaration();
        if (! decl)
            return false;

        Function *funTy = decl->type()->asFunctionType();
        if (! funTy)
            return false;

        else if (funTy->isPureVirtual())
            return false;

        return true;
    }

    virtual bool visit(ClassSpecifierAST *ast)
    {
        Class *klass = ast->symbol;
        const QByteArray className = id_cast(ast->name);

        Identifier *visit_id = control()->findOrInsertIdentifier("accept0");
        Symbol *accept0Method = klass->members()->lookat(visit_id);
        for (; accept0Method; accept0Method = accept0Method->next()) {
            if (accept0Method->identifier() != visit_id)
                continue;

            if (checkMethod(accept0Method))
                break;
        }

        if (! accept0Method)
            return true;

        classMap.insert(className, ast);

        *out
                << "void " << className.constData() << "::accept0(ASTVisitor *visitor)" << endl
                << "{" << endl
                << "    if (visitor->visit(this)) {" << endl;

        visitMembers(klass);

        *out
                << "    }" << endl
                << "    visitor->endVisit(this);" << endl
                << "}" << endl
                << endl;

        return true;
    }
};

class Match0CG: protected ASTVisitor
{
    QDir _cplusplusDir;
    QTextStream *out;

public:
    Match0CG(const QDir &cplusplusDir, Control *control)
        : ASTVisitor(control), _cplusplusDir(cplusplusDir), out(0)
    { }

    void operator()(AST *ast)
    {
        QFileInfo fileInfo(_cplusplusDir, QLatin1String("ASTMatch0.cpp"));

        QFile file(fileInfo.absoluteFilePath());
        if (! file.open(QFile::WriteOnly))
            return;

        QTextStream output(&file);
        out = &output;

        *out << copyrightHeader <<
            "\n"
            "#include \"AST.h\"\n"
            "#include \"ASTMatcher.h\"\n"
            "\n"
            "using namespace CPlusPlus;\n" << endl;

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
        Overview oo;
        const QString className = oo(klass->name());

        *out << "    if (" << className << " *_other = pattern->as" << className.left(className.length() - 3) << "())" << endl;

        *out << "        return matcher->match(this, _other);" << endl;

        *out << endl;
    }

    bool checkMethod(Symbol *accept0Method) const
    {
        Declaration *decl = accept0Method->asDeclaration();
        if (! decl)
            return false;

        Function *funTy = decl->type()->asFunctionType();
        if (! funTy)
            return false;

        else if (funTy->isPureVirtual())
            return false;

        return true;
    }

    virtual bool visit(ClassSpecifierAST *ast)
    {
        Class *klass = ast->symbol;
        const QByteArray className = id_cast(ast->name);

        Identifier *match0_id = control()->findOrInsertIdentifier("match0");
        Symbol *accept0Method = klass->members()->lookat(match0_id);
        for (; accept0Method; accept0Method = accept0Method->next()) {
            if (accept0Method->identifier() != match0_id)
                continue;

            if (checkMethod(accept0Method))
                break;
        }

        if (! accept0Method)
            return true;

        classMap.insert(className, ast);

        *out
                << "bool " << className.constData() << "::match0(AST *pattern, ASTMatcher *matcher)" << endl
                << "{" << endl;

        visitMembers(klass);

        *out
                << "    return false;" << endl
                << "}" << endl
                << endl;

        return true;
    }
};


class MatcherCPPCG: protected ASTVisitor
{
    QDir _cplusplusDir;
    QTextStream *out;

public:
    MatcherCPPCG(const QDir &cplusplusDir, Control *control)
        : ASTVisitor(control), _cplusplusDir(cplusplusDir), out(0)
    { }

    void operator()(AST *ast)
    {
        QFileInfo fileInfo(_cplusplusDir, QLatin1String("ASTMatcher.cpp"));

        QFile file(fileInfo.absoluteFilePath());
        if (! file.open(QFile::WriteOnly))
            return;

        QTextStream output(&file);
        out = &output;

        *out << copyrightHeader << endl
                << "#include \"AST.h\"" << endl
                << "#include \"ASTMatcher.h\"" << endl
                << "#include \"TranslationUnit.h\"" << endl
                << endl
                << "using namespace CPlusPlus;" << endl
                << endl
                << "ASTMatcher::ASTMatcher(TranslationUnit *translationUnit)"
                << "    : _translationUnit(translationUnit)" << endl
                << "{ }" << endl
                << endl
                << "ASTMatcher::~ASTMatcher()" << endl
                << "{ }" << endl
                << endl
                << "TranslationUnit *ASTMatcher::translationUnit() const" << endl
                << "{ return _translationUnit; }" << endl
                << endl;

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
        for (unsigned i = 0; i < klass->memberCount(); ++i) {
            Symbol *member = klass->memberAt(i);
            if (! member->name())
                continue;

            Identifier *id = member->name()->identifier();

            if (! id)
                continue;

            const QByteArray memberName = QByteArray::fromRawData(id->chars(), id->size());
            if (member->type().isUnsigned() && memberName.endsWith("_token")) {

                *out
                        << "    pattern->" << memberName << " = node->" << memberName << ";" << endl
                        << endl;

            } else if (PointerType *ptrTy = member->type()->asPointerType()) {

                if (NamedType *namedTy = ptrTy->elementType()->asNamedType()) {
                    QByteArray typeName = namedTy->name()->identifier()->chars();

                    if (typeName.endsWith("AST")) {
                        *out
                                << "    if (! pattern->" << memberName << ")" << endl
                                << "        pattern->" << memberName << " = node->" << memberName << ";" << endl
                                << "    else if (! AST::match(node->" << memberName << ", pattern->" << memberName << ", this))" << endl
                                << "        return false;" << endl
                                << endl;
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

    bool checkMethod(Symbol *accept0Method) const
    {
        Declaration *decl = accept0Method->asDeclaration();
        if (! decl)
            return false;

        Function *funTy = decl->type()->asFunctionType();
        if (! funTy)
            return false;

        else if (funTy->isPureVirtual())
            return false;

        return true;
    }

    virtual bool visit(ClassSpecifierAST *ast)
    {
        Class *klass = ast->symbol;
        const QByteArray className = id_cast(ast->name);

        Identifier *match0_id = control()->findOrInsertIdentifier("match0");
        Symbol *match0Method = klass->members()->lookat(match0_id);
        for (; match0Method; match0Method = match0Method->next()) {
            if (match0Method->identifier() != match0_id)
                continue;

            if (checkMethod(match0Method))
                break;
        }

        if (! match0Method)
            return true;

        classMap.insert(className, ast);

        *out
                << "bool ASTMatcher::match(" << className.constData() << " *node, " << className.constData() << " *pattern)" << endl
                << "{" << endl
                << "    (void) node;" << endl
                << "    (void) pattern;" << endl
                << endl;

        visitMembers(klass);

        *out
                << "    return true;" << endl
                << "}" << endl
                << endl;

        return true;
    }
};

class RemoveCastMethods: protected ASTVisitor
{
public:
    RemoveCastMethods(Document::Ptr doc, QTextDocument *document)
        : ASTVisitor(doc->control()), document(document) {}

    QList<QTextCursor> operator()(AST *ast)
    {
        _cursors.clear();
        accept(ast);
        return _cursors;
    }

protected:
    virtual bool visit(FunctionDefinitionAST *ast)
    {
        Function *fun = ast->symbol;
        const QString functionName = oo(fun->name());

        if (functionName.length() > 3 && functionName.startsWith(QLatin1String("as"))
            && functionName.at(2).isUpper()) {

            QTextCursor tc = createCursor(translationUnit(), ast, document);

            //qDebug() << qPrintable(tc.selectedText());
            _cursors.append(tc);
        }

        return true;
    }

private:
    QTextDocument *document;
    QList<QTextCursor> _cursors;
    Overview oo;
};


QStringList generateAST_H(const Snapshot &snapshot, const QDir &cplusplusDir)
{
    QStringList astDerivedClasses;

    QFileInfo fileAST_h(cplusplusDir, QLatin1String("AST.h"));
    Q_ASSERT(fileAST_h.exists());

    const QString fileName = fileAST_h.absoluteFilePath();

    QFile file(fileName);
    if (! file.open(QFile::ReadOnly))
        return astDerivedClasses;

    const QString source = QTextStream(&file).readAll();
    file.close();

    QTextDocument document;
    document.setPlainText(source);

    Document::Ptr doc = Document::create(fileName);
    const QByteArray preprocessedCode = snapshot.preprocessedCode(source, fileName);
    doc->setSource(preprocessedCode);
    doc->check();

    FindASTNodes process(doc, &document);
    astNodes = process(doc->translationUnit()->ast());

    RemoveCastMethods removeCastMethods(doc, &document);

    QList<QTextCursor> baseCastMethodCursors = removeCastMethods(astNodes.base);
    QMap<ClassSpecifierAST *, QList<QTextCursor> > cursors;
    QMap<ClassSpecifierAST *, QString> replacementCastMethods;

    Overview oo;

    QStringList castMethods;
    foreach (ClassSpecifierAST *classAST, astNodes.deriveds) {
        cursors[classAST] = removeCastMethods(classAST);
        const QString className = oo(classAST->symbol->name());
        const QString methodName = QLatin1String("as") + className.mid(0, className.length() - 3);
        replacementCastMethods[classAST] = QString("    virtual %1 *%2() { return this; }\n").arg(className, methodName);
        castMethods.append(QString("    virtual %1 *%2() { return 0; }\n").arg(className, methodName));

        astDerivedClasses.append(className);
    }

    if (! baseCastMethodCursors.isEmpty()) {
        castMethods.sort();
        for (int i = 0; i < baseCastMethodCursors.length(); ++i) {
            baseCastMethodCursors[i].removeSelectedText();
        }

        baseCastMethodCursors.first().insertText(castMethods.join(QLatin1String("")));
    }

    for (int classIndex = 0; classIndex < astNodes.deriveds.size(); ++classIndex) {
        ClassSpecifierAST *classAST = astNodes.deriveds.at(classIndex);

        // remove the cast methods.
        QList<QTextCursor> c = cursors.value(classAST);
        for (int i = 0; i < c.length(); ++i) {
            c[i].removeSelectedText();
        }

        astNodes.endOfPublicClassSpecifiers[classIndex].insertText(replacementCastMethods.value(classAST));
    }

    if (file.open(QFile::WriteOnly)) {
        QTextStream out(&file);
        out << document.toPlainText();
    }

    Accept0CG cg(cplusplusDir, doc->control());
    cg(doc->translationUnit()->ast());

    Match0CG cg2(cplusplusDir, doc->control());
    cg2(doc->translationUnit()->ast());

    MatcherCPPCG cg3(cplusplusDir, doc->control());
    cg3(doc->translationUnit()->ast());

    return astDerivedClasses;
}

class FindASTForwards: protected ASTVisitor
{
public:
    FindASTForwards(Document::Ptr doc, QTextDocument *document)
        : ASTVisitor(doc->control()), document(document)
    {}

    QList<QTextCursor> operator()(AST *ast)
    {
        accept(ast);
        return _cursors;
    }

protected:
    bool visit(SimpleDeclarationAST *ast)
    {
        if (! ast->decl_specifier_list)
            return false;

        if (ElaboratedTypeSpecifierAST *e = ast->decl_specifier_list->value->asElaboratedTypeSpecifier()) {
            if (tokenKind(e->classkey_token) == T_CLASS && !ast->declarator_list) {
                QString className = oo(e->name->name);

                if (className.length() > 3 && className.endsWith(QLatin1String("AST"))) {
                    QTextCursor tc = createCursor(translationUnit(), ast, document);
                    _cursors.append(tc);
                }
            }
        }

        return true;
    }

private:
    QTextDocument *document;
    QList<QTextCursor> _cursors;
    Overview oo;
};

void generateASTFwd_h(const Snapshot &snapshot, const QDir &cplusplusDir, const QStringList &astDerivedClasses)
{
    QFileInfo fileASTFwd_h(cplusplusDir, QLatin1String("ASTfwd.h"));
    Q_ASSERT(fileASTFwd_h.exists());

    const QString fileName = fileASTFwd_h.absoluteFilePath();

    QFile file(fileName);
    if (! file.open(QFile::ReadOnly))
        return;

    const QString source = QTextStream(&file).readAll();
    file.close();

    QTextDocument document;
    document.setPlainText(source);

    Document::Ptr doc = Document::create(fileName);
    const QByteArray preprocessedCode = snapshot.preprocessedCode(source, fileName);
    doc->setSource(preprocessedCode);
    doc->check();

    FindASTForwards process(doc, &document);
    QList<QTextCursor> cursors = process(doc->translationUnit()->ast());

    for (int i = 0; i < cursors.length(); ++i)
        cursors[i].removeSelectedText();

    QString replacement;
    foreach (const QString &astDerivedClass, astDerivedClasses) {
        replacement += QString(QLatin1String("class %1;\n")).arg(astDerivedClass);
    }

    cursors.first().insertText(replacement);

    if (file.open(QFile::WriteOnly)) {
        QTextStream out(&file);
        out << document.toPlainText();
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList files = app.arguments();
    files.removeFirst();

    if (files.isEmpty()) {
        std::cerr << "Usage: cplusplus [path to C++ front-end]" << std::endl;
        return EXIT_FAILURE;
    }

    QDir cplusplusDir(files.first());
    Snapshot snapshot;

    QStringList astDerivedClasses = generateAST_H(snapshot, cplusplusDir);
    astDerivedClasses.sort();
    generateASTFwd_h(snapshot, cplusplusDir, astDerivedClasses);
}
