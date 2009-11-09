
#include <QCoreApplication>
#include <QStringList>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
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

                unsigned endLine, endColumn;
                getTokenEndPosition(accessDeclaration->lastToken() - 1, &endLine, &endColumn);

                QTextCursor tc(document);
                tc.setPosition(document->findBlockByNumber(endLine - 1).position() + endColumn - 1);

                int charsToSkip = 0;
                forever {
                    QChar ch = document->characterAt(tc.position() + charsToSkip);
                    if (! ch.isSpace())
                        break;

                    ++charsToSkip;

                    if (ch == QChar::ParagraphSeparator)
                        break;
                }

                tc.setPosition(tc.position() + charsToSkip);

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
            unsigned startLine, startColumn, endLine, endColumn;
            getTokenStartPosition(ast->firstToken(), &startLine, &startColumn);
            getTokenEndPosition(ast->lastToken() - 1, &endLine, &endColumn);

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

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList files = app.arguments();
    files.removeFirst();

    if (files.isEmpty()) {
        std::cerr << "Usage: cplusplus AST.h" << std::endl;
        return EXIT_FAILURE;
    }

    Snapshot snapshot;

    //const QString configuration = QLatin1String("#define CPLUSPLUS_EXPORT\n");

    foreach (const QString &fileName, files) {
        QFile file(fileName);
        if (! file.open(QFile::ReadOnly))
            continue;

        const QString source = QTextStream(&file).readAll();
        QTextDocument document;
        document.setPlainText(source);

        Document::Ptr doc = Document::create(fileName);
        //doc->control()->setDiagnosticClient(0);
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
            castMethods.append(QString("    virtual %1 *%2() { return 0; }").arg(className, methodName));
        }

        if (! baseCastMethodCursors.isEmpty()) {
            castMethods.sort();
            for (int i = 0; i < baseCastMethodCursors.length(); ++i) {
                baseCastMethodCursors[i].removeSelectedText();
            }

            baseCastMethodCursors.first().insertText(castMethods.join(QLatin1String("\n")));
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

        std::cout << qPrintable(document.toPlainText());
    }
}
