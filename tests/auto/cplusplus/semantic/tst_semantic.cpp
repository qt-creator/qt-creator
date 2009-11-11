
#include <QtTest>
#include <QtDebug>
#include <QTextDocument>
#include <QTextCursor>

#include <Control.h>
#include <Parser.h>
#include <AST.h>
#include <Semantic.h>
#include <Scope.h>
#include <Symbols.h>
#include <CoreTypes.h>
#include <Names.h>
#include <Literals.h>
#include <DiagnosticClient.h>
#include <GenTemplateInstance.h>
#include <Overview.h>
#include <ExpressionUnderCursor.h>

using namespace CPlusPlus;

class tst_Semantic: public QObject
{
    Q_OBJECT

    Control control;

public:
    tst_Semantic()
    { control.setDiagnosticClient(&diag); }

    TranslationUnit *parse(const QByteArray &source,
                           TranslationUnit::ParseMode mode,
                           bool enableObjc)
    {
        StringLiteral *fileId = control.findOrInsertStringLiteral("<stdin>");
        TranslationUnit *unit = new TranslationUnit(&control, fileId);
        unit->setSource(source.constData(), source.length());
        unit->setObjCEnabled(enableObjc);
        unit->parse(mode);
        return unit;
    }

    class Document {
        Q_DISABLE_COPY(Document)

    public:
        Document(TranslationUnit *unit)
            : unit(unit), globals(new Scope()), errorCount(0)
        { }

        ~Document()
        { delete globals; }

        void check()
        {
            QVERIFY(unit);
            QVERIFY(unit->ast());
            Semantic sem(unit->control());
            TranslationUnitAST *ast = unit->ast()->asTranslationUnit();
            QVERIFY(ast);
            for (DeclarationListAST *decl = ast->declaration_list; decl; decl = decl->next) {
                sem.check(decl->value, globals);
            }
        }

        TranslationUnit *unit;
        Scope *globals;
        unsigned errorCount;
    };

    class Diagnostic: public DiagnosticClient {
    public:
        int errorCount;

        Diagnostic()
            : errorCount(0)
        { }

        virtual void report(int /*level*/,
                            StringLiteral *fileName,
                            unsigned line, unsigned column,
                            const char *format, va_list ap)
        {
            ++errorCount;

            qDebug() << fileName->chars()<<':'<<line<<':'<<column<<' '<<QString().sprintf(format, ap);
        }
    };

    Diagnostic diag;


    QSharedPointer<Document> document(const QByteArray &source, bool enableObjc = false)
    {
        diag.errorCount = 0; // reset the error count.
        TranslationUnit *unit = parse(source, TranslationUnit::ParseTranlationUnit, enableObjc);
        QSharedPointer<Document> doc(new Document(unit));
        doc->check();
        doc->errorCount = diag.errorCount;
        return doc;
    }

private slots:
    void function_declaration_1();
    void function_declaration_2();
    void function_definition_1();
    void nested_class_1();
    void typedef_1();
    void typedef_2();
    void typedef_3();
    void const_1();
    void const_2();
    void pointer_to_function_1();

    void template_instance_1();

    void expression_under_cursor_1();

    void bracketed_expression_under_cursor_1();
    void bracketed_expression_under_cursor_2();
    void bracketed_expression_under_cursor_3();
    void bracketed_expression_under_cursor_4();
    void bracketed_expression_under_cursor_5();
    void bracketed_expression_under_cursor_6();
    void bracketed_expression_under_cursor_7();
    void bracketed_expression_under_cursor_8();

    void objcClass_1();
};

void tst_Semantic::function_declaration_1()
{
    QSharedPointer<Document> doc = document("void foo();");
    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->symbolCount(), 1U);

    Declaration *decl = doc->globals->symbolAt(0)->asDeclaration();
    QVERIFY(decl);

    FullySpecifiedType declTy = decl->type();
    Function *funTy = declTy->asFunctionType();
    QVERIFY(funTy);
    QVERIFY(funTy->returnType()->isVoidType());
    QCOMPARE(funTy->argumentCount(), 0U);

    QVERIFY(decl->name()->isNameId());
    Identifier *funId = decl->name()->asNameId()->identifier();
    QVERIFY(funId);

    const QByteArray foo(funId->chars(), funId->size());
    QCOMPARE(foo, QByteArray("foo"));
}

void tst_Semantic::function_declaration_2()
{
    QSharedPointer<Document> doc = document("void foo(const QString &s);");
    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->symbolCount(), 1U);

    Declaration *decl = doc->globals->symbolAt(0)->asDeclaration();
    QVERIFY(decl);

    FullySpecifiedType declTy = decl->type();
    Function *funTy = declTy->asFunctionType();
    QVERIFY(funTy);
    QVERIFY(funTy->returnType()->isVoidType());
    QCOMPARE(funTy->argumentCount(), 1U);

    // check the formal argument.
    Argument *arg = funTy->argumentAt(0)->asArgument();
    QVERIFY(arg);
    QVERIFY(arg->name());
    QVERIFY(! arg->hasInitializer());

    // check the argument's name.
    NameId *argNameId = arg->name()->asNameId();
    QVERIFY(argNameId);

    Identifier *argId = argNameId->identifier();
    QVERIFY(argId);

    QCOMPARE(QByteArray(argId->chars(), argId->size()), QByteArray("s"));

    // check the type of the formal argument
    FullySpecifiedType argTy = arg->type();
    QVERIFY(argTy->isReferenceType());
    QVERIFY(argTy->asReferenceType()->elementType().isConst());
    NamedType *namedTy = argTy->asReferenceType()->elementType()->asNamedType();
    QVERIFY(namedTy);
    QVERIFY(namedTy->name());
    Identifier *namedTypeId = namedTy->name()->asNameId()->identifier();
    QVERIFY(namedTypeId);
    QCOMPARE(QByteArray(namedTypeId->chars(), namedTypeId->size()),
             QByteArray("QString"));

    QVERIFY(decl->name()->isNameId());
    Identifier *funId = decl->name()->asNameId()->identifier();
    QVERIFY(funId);

    const QByteArray foo(funId->chars(), funId->size());
    QCOMPARE(foo, QByteArray("foo"));
}

void tst_Semantic::function_definition_1()
{
    QSharedPointer<Document> doc = document("void foo() {}");
    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->symbolCount(), 1U);

    Function *funTy = doc->globals->symbolAt(0)->asFunction();
    QVERIFY(funTy);
    QVERIFY(funTy->returnType()->isVoidType());
    QCOMPARE(funTy->argumentCount(), 0U);

    QVERIFY(funTy->name()->isNameId());
    Identifier *funId = funTy->name()->asNameId()->identifier();
    QVERIFY(funId);

    const QByteArray foo(funId->chars(), funId->size());
    QCOMPARE(foo, QByteArray("foo"));
}

void tst_Semantic::nested_class_1()
{
    QSharedPointer<Document> doc = document(
"class Object {\n"
"    class Data;\n"
"    Data *d;\n"
"};\n"
"class Object::Data {\n"
"   Object *q;\n"
"};\n"
    );
    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->symbolCount(), 2U);

    Class *classObject = doc->globals->symbolAt(0)->asClass();
    QVERIFY(classObject);
    QVERIFY(classObject->name());
    NameId *classObjectNameId = classObject->name()->asNameId();
    QVERIFY(classObjectNameId);
    Identifier *objectId = classObjectNameId->identifier();
    QCOMPARE(QByteArray(objectId->chars(), objectId->size()), QByteArray("Object"));
    QCOMPARE(classObject->baseClassCount(), 0U);
    QCOMPARE(classObject->members()->symbolCount(), 2U);

    Class *classObjectData = doc->globals->symbolAt(1)->asClass();
    QVERIFY(classObjectData);
    QVERIFY(classObjectData->name());
    QualifiedNameId *q = classObjectData->name()->asQualifiedNameId();
    QVERIFY(q);
    QCOMPARE(q->nameCount(), 2U);
    QVERIFY(q->nameAt(0)->asNameId());
    QVERIFY(q->nameAt(1)->asNameId());
    QCOMPARE(q->nameAt(0), classObject->name());
    QCOMPARE(doc->globals->lookat(q->nameAt(0)->asNameId()->identifier()), classObject);

    Declaration *decl = classObjectData->memberAt(0)->asDeclaration();
    QVERIFY(decl);
    PointerType *ptrTy = decl->type()->asPointerType();
    QVERIFY(ptrTy);
    NamedType *namedTy = ptrTy->elementType()->asNamedType();
    QVERIFY(namedTy);
    QVERIFY(namedTy->name()->asNameId());
    QCOMPARE(namedTy->name()->asNameId()->identifier(), objectId);
}

void tst_Semantic::typedef_1()
{
    QSharedPointer<Document> doc = document(
"typedef struct {\n"
"   int x, y;\n"
"} Point;\n"
"int main() {\n"
"   Point pt;\n"
"   pt.x = 1;\n"
"}\n"
    );

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->symbolCount(), 3U);

    Class *anonStruct = doc->globals->symbolAt(0)->asClass();
    QVERIFY(anonStruct);
    QCOMPARE(anonStruct->memberCount(), 2U);

    Declaration *typedefPointDecl = doc->globals->symbolAt(1)->asDeclaration();
    QVERIFY(typedefPointDecl);
    QVERIFY(typedefPointDecl->isTypedef());
    QCOMPARE(typedefPointDecl->type()->asClassType(), anonStruct);

    Function *mainFun = doc->globals->symbolAt(2)->asFunction();
    QVERIFY(mainFun);
}

void tst_Semantic::typedef_2()
{
    QSharedPointer<Document> doc = document(
"struct _Point {\n"
"   int x, y;\n"
"};\n"
"typedef _Point Point;\n"
"int main() {\n"
"   Point pt;\n"
"   pt.x = 1;\n"
"}\n"
    );

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->symbolCount(), 3U);

    Class *_pointStruct= doc->globals->symbolAt(0)->asClass();
    QVERIFY(_pointStruct);
    QCOMPARE(_pointStruct->memberCount(), 2U);

    Declaration *typedefPointDecl = doc->globals->symbolAt(1)->asDeclaration();
    QVERIFY(typedefPointDecl);
    QVERIFY(typedefPointDecl->isTypedef());
    QVERIFY(typedefPointDecl->type()->isNamedType());
    QCOMPARE(typedefPointDecl->type()->asNamedType()->name(), _pointStruct->name());

    Function *mainFun = doc->globals->symbolAt(2)->asFunction();
    QVERIFY(mainFun);
}

void tst_Semantic::typedef_3()
{
    QSharedPointer<Document> doc = document(
"typedef struct {\n"
"   int x, y;\n"
"} *PointPtr;\n"
    );

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->symbolCount(), 2U);

    Class *_pointStruct= doc->globals->symbolAt(0)->asClass();
    QVERIFY(_pointStruct);
    QCOMPARE(_pointStruct->memberCount(), 2U);

    Declaration *typedefPointDecl = doc->globals->symbolAt(1)->asDeclaration();
    QVERIFY(typedefPointDecl);
    QVERIFY(typedefPointDecl->isTypedef());
    QVERIFY(typedefPointDecl->type()->isPointerType());
    QCOMPARE(typedefPointDecl->type()->asPointerType()->elementType()->asClassType(),
             _pointStruct);
}

void tst_Semantic::const_1()
{
    QSharedPointer<Document> doc = document("\n"
"int foo(const int *s);\n"
    );

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->symbolCount(), 1U);

    Declaration *decl = doc->globals->symbolAt(0)->asDeclaration();
    QVERIFY(decl);
    QVERIFY(decl->type()->isFunctionType());
    Function *funTy = decl->type()->asFunctionType();
    QVERIFY(funTy->returnType()->isIntegerType());
    QCOMPARE(funTy->argumentCount(), 1U);
    Argument *arg = funTy->argumentAt(0)->asArgument();
    QVERIFY(arg);
    QVERIFY(! arg->type().isConst());
    QVERIFY(arg->type()->isPointerType());
    QVERIFY(arg->type()->asPointerType()->elementType().isConst());
    QVERIFY(arg->type()->asPointerType()->elementType()->isIntegerType());
}

void tst_Semantic::const_2()
{
    QSharedPointer<Document> doc = document("\n"
"int foo(char * const s);\n"
    );

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->symbolCount(), 1U);

    Declaration *decl = doc->globals->symbolAt(0)->asDeclaration();
    QVERIFY(decl);
    QVERIFY(decl->type()->isFunctionType());
    Function *funTy = decl->type()->asFunctionType();
    QVERIFY(funTy->returnType()->isIntegerType());
    QCOMPARE(funTy->argumentCount(), 1U);
    Argument *arg = funTy->argumentAt(0)->asArgument();
    QVERIFY(arg);
    QVERIFY(arg->type().isConst());
    QVERIFY(arg->type()->isPointerType());
    QVERIFY(! arg->type()->asPointerType()->elementType().isConst());
    QVERIFY(arg->type()->asPointerType()->elementType()->isIntegerType());
}

void tst_Semantic::pointer_to_function_1()
{
    QSharedPointer<Document> doc = document("void (*QtSomething)();");
    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->symbolCount(), 1U);

    Declaration *decl = doc->globals->symbolAt(0)->asDeclaration();
    QVERIFY(decl);

    PointerType *ptrTy = decl->type()->asPointerType();
    QVERIFY(ptrTy);

    Function *funTy = ptrTy->elementType()->asFunctionType();
    QVERIFY(funTy);

    QEXPECT_FAIL("", "Requires initialize enclosing scope of pointer-to-function symbols", Continue);
    QVERIFY(funTy->scope());

    QEXPECT_FAIL("", "Requires initialize enclosing scope of pointer-to-function symbols", Continue);
    QCOMPARE(funTy->scope(), decl->scope());
}

void tst_Semantic::template_instance_1()
{
    QSharedPointer<Document> doc = document("void append(const _Tp &value);");
    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->symbolCount(), 1U);

    Declaration *decl = doc->globals->symbolAt(0)->asDeclaration();
    QVERIFY(decl);

    GenTemplateInstance::Substitution subst;
    Identifier *nameTp = control.findOrInsertIdentifier("_Tp");
    FullySpecifiedType intTy(control.integerType(IntegerType::Int));
    subst.append(qMakePair(nameTp, intTy));

    GenTemplateInstance inst(&control, subst);
    FullySpecifiedType genTy = inst(decl);

    Overview oo;
    oo.setShowReturnTypes(true);

    const QString genDecl = oo.prettyType(genTy);
    QCOMPARE(genDecl, QString::fromLatin1("void(const int &)"));
}

void tst_Semantic::expression_under_cursor_1()
{
    const QString plainText = "void *ptr = foo(10, bar";

    QTextDocument textDocument;
    textDocument.setPlainText(plainText);

    QTextCursor tc(&textDocument);
    tc.movePosition(QTextCursor::End);

    ExpressionUnderCursor expressionUnderCursor;
    const QString expression = expressionUnderCursor(tc);

    QCOMPARE(expression, QString("bar"));
}

void tst_Semantic::bracketed_expression_under_cursor_1()
{
    const QString plainText = "int i = 0, j[1], k = j[i";

    QTextDocument textDocument;
    textDocument.setPlainText(plainText);

    QTextCursor tc(&textDocument);
    tc.movePosition(QTextCursor::End);

    ExpressionUnderCursor expressionUnderCursor;
    const QString expression = expressionUnderCursor(tc);

    QCOMPARE(expression, QString("i"));
}

void tst_Semantic::bracketed_expression_under_cursor_2()
{
    const QString plainText = "[receiver msg";

    QTextDocument textDocument;
    textDocument.setPlainText(plainText);

    QTextCursor tc(&textDocument);
    tc.movePosition(QTextCursor::End);

    ExpressionUnderCursor expressionUnderCursor;
    const QString expression = expressionUnderCursor(tc);

    QCOMPARE(expression, plainText);
}

void tst_Semantic::bracketed_expression_under_cursor_3()
{
    const QString plainText = "[receiver msgParam1:0 msgParam2";

    QTextDocument textDocument;
    textDocument.setPlainText(plainText);

    QTextCursor tc(&textDocument);
    tc.movePosition(QTextCursor::End);

    ExpressionUnderCursor expressionUnderCursor;
    const QString expression = expressionUnderCursor(tc);

    QCOMPARE(expression, plainText);
}

void tst_Semantic::bracketed_expression_under_cursor_4()
{
    const QString plainText = "[receiver msgParam1:0 msgParam2:@\"zoo\" msgParam3";

    QTextDocument textDocument;
    textDocument.setPlainText(plainText);

    QTextCursor tc(&textDocument);
    tc.movePosition(QTextCursor::End);

    ExpressionUnderCursor expressionUnderCursor;
    const QString expression = expressionUnderCursor(tc);

    QCOMPARE(expression, plainText);
}

void tst_Semantic::bracketed_expression_under_cursor_5()
{
    const QString plainText = "if ([receiver message";

    QTextDocument textDocument;
    textDocument.setPlainText(plainText);

    QTextCursor tc(&textDocument);
    tc.movePosition(QTextCursor::End);

    ExpressionUnderCursor expressionUnderCursor;
    const QString expression = expressionUnderCursor(tc);

    QCOMPARE(expression, QString("[receiver message"));
}

void tst_Semantic::bracketed_expression_under_cursor_6()
{
    const QString plainText = "if ([receiver msgParam1:1 + i[1] msgParam2";

    QTextDocument textDocument;
    textDocument.setPlainText(plainText);

    QTextCursor tc(&textDocument);
    tc.movePosition(QTextCursor::End);

    ExpressionUnderCursor expressionUnderCursor;
    const QString expression = expressionUnderCursor(tc);

    QCOMPARE(expression, QString("[receiver msgParam1:1 + i[1] msgParam2"));
}

void tst_Semantic::bracketed_expression_under_cursor_7()
{
    const QString plainText = "int i = 0, j[1], k = j[(i == 0) ? 0 : i";

    QTextDocument textDocument;
    textDocument.setPlainText(plainText);

    QTextCursor tc(&textDocument);
    tc.movePosition(QTextCursor::End);

    ExpressionUnderCursor expressionUnderCursor;
    const QString expression = expressionUnderCursor(tc);

    QCOMPARE(expression, QString("i"));
}

void tst_Semantic::bracketed_expression_under_cursor_8()
{
    const QString plainText = "[[receiver msg] param1:[receiver msg] param2";

    QTextDocument textDocument;
    textDocument.setPlainText(plainText);

    QTextCursor tc(&textDocument);
    tc.movePosition(QTextCursor::End);

    ExpressionUnderCursor expressionUnderCursor;
    const QString expression = expressionUnderCursor(tc);

    QCOMPARE(expression, plainText);
}

void tst_Semantic::objcClass_1()
{
    QSharedPointer<Document> doc = document("\n"
                                            "@interface Zoo {} +(id)alloc;-(id)init;@end\n"
                                            "@implementation Zoo\n"
                                            "+(id)alloc{}\n"
                                            "-(id)init{}\n"
                                            "-(void)dealloc{}\n"
                                            "@end\n",
                                            true);

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->symbolCount(), 2U);

    ObjCClass *iface = doc->globals->symbolAt(0)->asObjCClass();
    QVERIFY(iface);
    QVERIFY(iface->isInterface());
    QCOMPARE(iface->memberCount(), 2U);

    ObjCClass *impl = doc->globals->symbolAt(1)->asObjCClass();
    QVERIFY(impl);
    QVERIFY(!impl->isInterface());
    QCOMPARE(impl->memberCount(), 3U);

    ObjCMethod *allocMethod = impl->memberAt(0)->asObjCMethod();
    QVERIFY(allocMethod);
    QVERIFY(allocMethod->name() && allocMethod->name()->identifier());
    QCOMPARE(QLatin1String(allocMethod->name()->identifier()->chars()), QLatin1String("alloc"));
    QVERIFY(allocMethod->isStatic());

    ObjCMethod *deallocMethod = impl->memberAt(2)->asObjCMethod();
    QVERIFY(deallocMethod);
    QVERIFY(deallocMethod->name() && deallocMethod->name()->identifier());
    QCOMPARE(QLatin1String(deallocMethod->name()->identifier()->chars()), QLatin1String("dealloc"));
    QVERIFY(!deallocMethod->isStatic());
}

QTEST_APPLESS_MAIN(tst_Semantic)
#include "tst_semantic.moc"
