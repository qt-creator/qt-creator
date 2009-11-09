
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

#include <iostream>
#include <cstdlib>

using namespace CPlusPlus;

QTextCursor createCursor(TranslationUnit *unit, AST *ast, QTextDocument *document)
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

class ASTNodes
{
public:
    ASTNodes(): base(0) {}

    ClassSpecifierAST *base; // points to "class AST"
    QList<ClassSpecifierAST *> deriveds; // n where n extends AST
    QList<QTextCursor> endOfPublicClassSpecifiers;
};

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
                for (DeclarationListAST *it = ast->member_specifiers; it; it = it->next) {
                    if (AccessDeclarationAST *decl = it->declaration->asAccessDeclaration()) {
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
    ASTNodes astNodes = process(doc->translationUnit()->ast());

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
        if (ElaboratedTypeSpecifierAST *e = ast->decl_specifier_seq->asElaboratedTypeSpecifier()) {
            if (tokenKind(e->classkey_token) == T_CLASS && !ast->declarators) {
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
