/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmljsfindexportedcpptypes.h"

#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsdocument.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/cppmodelmanagerbase.h>
#include <cplusplus/CppDocument.h>
#include <utils/qtcassert.h>

#include <QList>
#include <QRegExp>

//using namespace QmlJS;

namespace {
using namespace CPlusPlus;

class ExportedQmlType {
public:
    QString packageName;
    QString typeName;
    LanguageUtils::ComponentVersion version;
    Scope *scope;
    QString typeExpression;
    QString url;
    bool isCreatable;
    bool isSingleton;
};

class ContextProperty {
public:
    QString name;
    QString expression;
    unsigned line, column;
};

class FindExportsVisitor : protected ASTVisitor
{
    CPlusPlus::Document::Ptr _doc;
    QList<ExportedQmlType> _exportedTypes;
    QList<ContextProperty> _contextProperties;
    CompoundStatementAST *_compound;
    ASTMatcher _matcher;
    ASTPatternBuilder _builder;
    Overview _overview;
    QList<CPlusPlus::Document::DiagnosticMessage> _messages;

public:
    enum RegistrationFunction {
        InvalidRegistrationFunction,
        // template<typename T> int qmlRegisterType(const char *uri, int versionMajor, int versionMinor, const char *qmlName);
        // or template<typename T, int metaObjectRevision> int qmlRegisterType(const char *uri, int versionMajor, int versionMinor, const char *qmlName);
        QmlRegisterType4,
        // int qmlRegisterType(const QUrl & url, const char * uri, int versionMajor, int versionMinor, const char * qmlName)
        QmlRegisterType5,
        // template<typename T> int qmlRegisterSingletonType(const char * uri, int versionMajor, int versionMinor, const char * typeName, QObject *(* ) ( QQmlEngine *, QJSEngine * ) callback)
        QmlRegisterSingletonTypeCallback1,
        // int qmlRegisterSingletonType(const char * uri, int versionMajor, int versionMinor, const char * typeName, QJSValue(* ) ( QQmlEngine *, QJSEngine * ) callback)
        QmlRegisterSingletonTypeCallback2,
        // int qmlRegisterSingletonType(const QUrl &url, const char *uri, int versionMajor, int versionMinor, const char *qmlName)
        QmlRegisterSingletonTypeUrl,
        // template<typename T> int qmlRegisterUncreatableType(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& reason)
        // or template<typename T, int metaObjectRevision> int qmlRegisterUncreatableType(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& reason)
        QmlRegisterUncreatableType
    };

    FindExportsVisitor(CPlusPlus::Document::Ptr doc)
        : ASTVisitor(doc->translationUnit())
        , _doc(doc)
        , _compound(0)
    {}

    void operator()()
    {
        _exportedTypes.clear();
        _contextProperties.clear();
        accept(translationUnit()->ast());
    }

    QList<CPlusPlus::Document::DiagnosticMessage> messages() const
    {
        return _messages;
    }

    QList<ExportedQmlType> exportedTypes() const
    {
        return _exportedTypes;
    }

    QList<ContextProperty> contextProperties() const
    {
        return _contextProperties;
    }

protected:
    virtual bool visit(CompoundStatementAST *ast)
    {
        CompoundStatementAST *old = _compound;
        _compound = ast;
        accept(ast->statement_list);
        _compound = old;
        return false;
    }

    virtual bool visit(CallAST *ast)
    {
        if (checkForQmlRegisterType(ast))
            return false;
        checkForSetContextProperty(ast);
        return false;
    }

    bool checkForQmlRegisterType(CallAST *ast)
    {
        IdExpressionAST *idExp = ast->base_expression->asIdExpression();
        if (!idExp || !idExp->name)
            return false;
        RegistrationFunction registrationFunction = InvalidRegistrationFunction;
        TypeIdAST *typeId = 0;
        if (TemplateIdAST *templateId = idExp->name->asTemplateId()) {
            if (!templateId->identifier_token)
                return false;

            // check the name
            const Identifier *templateIdentifier = translationUnit()->identifier(templateId->identifier_token);
            if (!templateIdentifier)
                return false;
            const QString callName = QString::fromUtf8(templateIdentifier->chars());
            if (callName == QLatin1String("qmlRegisterType"))
                registrationFunction = QmlRegisterType4;
            else if (callName == QLatin1String("qmlRegisterSingletonType"))
                registrationFunction = QmlRegisterSingletonTypeCallback1;
            else if (callName == QLatin1String("qmlRegisterUncreatableType"))
                registrationFunction = QmlRegisterUncreatableType;
            else
                return false;

            // check that there is a typeId
            if (!templateId->template_argument_list || !templateId->template_argument_list->value)
                return false;
            // sometimes there can be a second argument, the metaRevisionNumber
            if (templateId->template_argument_list->next) {
                if (!templateId->template_argument_list->next->value ||
                        templateId->template_argument_list->next->next)
                    return false;
                // should just check for a generic ExpressionAST?
                NumericLiteralAST *metaRevision =
                        templateId->template_argument_list->next->value->asNumericLiteral();
                if (!metaRevision)
                    return false;
            }

            typeId = templateId->template_argument_list->value->asTypeId();
            if (!typeId)
                return false;
        } else if (idExp->name) {
            QByteArray fName;
            if (SimpleNameAST *simpleName = idExp->name->asSimpleName()) {
                if (const Identifier *id = translationUnit()->identifier(simpleName->identifier_token))
                    fName = QByteArray(id->chars(), id->size());
            }
            if (fName == "qmlRegisterType")
                registrationFunction = QmlRegisterType5;
            else if (fName == "qmlRegisterSingletonType")
                registrationFunction = QmlRegisterSingletonTypeCallback2;
            else
                return false;
        } else {
            return false;
        }
        // must have at least four arguments
        if (!ast->expression_list
                || !ast->expression_list->value || !ast->expression_list->next
                || !ast->expression_list->next->value || !ast->expression_list->next->next
                || !ast->expression_list->next->next->value || !ast->expression_list->next->next->next
                || !ast->expression_list->next->next->next->value)
            return false;
        switch (registrationFunction) {
        case InvalidRegistrationFunction:
            return false;
        case QmlRegisterType4:
            break;
        case QmlRegisterType5:
        case QmlRegisterSingletonTypeCallback1:
        case QmlRegisterSingletonTypeCallback2:
        case QmlRegisterSingletonTypeUrl:
        case QmlRegisterUncreatableType:
            if (!ast->expression_list->next->next->next->next
                    || !ast->expression_list->next->next->next->next->value
                    || ast->expression_list->next->next->next->next->next)
                return false;
        }
        ExpressionAST *uriExp = 0;
        ExpressionAST *majorVersionExp = 0;
        ExpressionAST *minorVersionExp = 0;
        ExpressionAST *nameExp = 0;
        if (registrationFunction == QmlRegisterType5) {
            uriExp = ast->expression_list->next->value;
            majorVersionExp = ast->expression_list->next->next->value;
            minorVersionExp = ast->expression_list->next->next->next->value;
            nameExp = ast->expression_list->next->next->next->next->value;
        } else if (registrationFunction == QmlRegisterSingletonTypeCallback2
                   && (!ast->expression_list->next->value->asNumericLiteral()
                       || ast->expression_list->next->next->next->value->asNumericLiteral())) {
            // discriminate between QmlRegisterSingletonTypeCallback2 and QmlRegisterSingletonTypeUrl,
            // this is very rough check, improve?
            registrationFunction = QmlRegisterSingletonTypeUrl;
            uriExp = ast->expression_list->next->value;
            majorVersionExp = ast->expression_list->next->next->value;
            minorVersionExp = ast->expression_list->next->next->next->value;
            nameExp = ast->expression_list->next->next->next->next->value;
        } else {
            uriExp = ast->expression_list->value;
            majorVersionExp = ast->expression_list->next->value;
            minorVersionExp = ast->expression_list->next->next->value;
            nameExp = ast->expression_list->next->next->next->value;
        }
        const StringLiteral *nameLit = 0;
        if (StringLiteralAST *nameAst = skipStringCall(nameExp)->asStringLiteral())
            nameLit = translationUnit()->stringLiteral(nameAst->literal_token);
        if (!nameLit) {
            unsigned line, column;
            translationUnit()->getTokenStartPosition(nameExp->firstToken(), &line, &column);
            _messages += Document::DiagnosticMessage(
                        Document::DiagnosticMessage::Warning,
                        _doc->fileName(),
                        line, column,
                        QmlJS::FindExportedCppTypes::tr(
                            "The type will only be available in the QML editors when the type name is a string literal"));
            return false;
        }

        // if the uri is a string literal, things are easy
        QString packageName;
        if (StringLiteralAST *packageAst = skipStringCall(uriExp)->asStringLiteral()) {
            const StringLiteral *packageLit = translationUnit()->stringLiteral(packageAst->literal_token);
            packageName = QString::fromUtf8(packageLit->chars(), packageLit->size());
        }
        // as a special case, allow an identifier package argument if there's a
        // Q_ASSERT(QLatin1String(uri) == QLatin1String("actual uri"));
        // in the enclosing compound statement
        QString uriNameString;
        if (IdExpressionAST *uriName = uriExp->asIdExpression())
            uriNameString = stringOf(uriName);
        if (packageName.isEmpty() && !uriNameString.isEmpty() && _compound) {
            for (StatementListAST *it = _compound->statement_list; it; it = it->next) {
                StatementAST *stmt = it->value;

                packageName = nameOfUriAssert(stmt, uriNameString);
                if (!packageName.isEmpty())
                    break;
            }
        }
        if (packageName.isEmpty() && _compound) {
            // check the comments in _compound for annotations
            QRegExp uriAnnotation(QLatin1String("@uri\\s*([\\w\\.]*)"));

            // scan every comment between the pipes in
            // {|
            //    // comment
            //    othercode;
            //   |qmlRegisterType<Foo>(...);
            const Token begin = _doc->translationUnit()->tokenAt(_compound->firstToken());
            const Token end = _doc->translationUnit()->tokenAt(ast->firstToken());

            // go through comments backwards to find the annotation closest to the call
            for (unsigned i = _doc->translationUnit()->commentCount(); i-- > 0; ) {
                const Token commentToken = _doc->translationUnit()->commentAt(i);
                if (commentToken.utf16charsBegin() >= end.utf16charsBegin()
                        || commentToken.utf16charsEnd() <= begin.utf16charsBegin()) {
                    continue;
                }
                const QString comment = stringOf(commentToken);
                if (uriAnnotation.indexIn(comment) != -1) {
                    packageName = uriAnnotation.cap(1);
                    break;
                }
            }
        }
        if (packageName.isEmpty()) {
            packageName = QmlJS::CppQmlTypes::defaultPackage;
            unsigned line, column;
            translationUnit()->getTokenStartPosition(ast->firstToken(), &line, &column);
            _messages += Document::DiagnosticMessage(
                        Document::DiagnosticMessage::Warning,
                        _doc->fileName(),
                        line, column,
                        QmlJS::FindExportedCppTypes::tr(
                            "The module URI cannot be determined by static analysis. The type will be available\n"
                            "globally in the QML editor. You can add a \"// @uri My.Module.Uri\" annotation to let\n"
                            "the QML editor know about a likely URI."));
        }

        // version arguments must be integer literals
        const NumericLiteral *majorLit = 0;
        const NumericLiteral *minorLit = 0;
        if (NumericLiteralAST *majorAst = majorVersionExp->asNumericLiteral())
            majorLit = translationUnit()->numericLiteral(majorAst->literal_token);
        if (NumericLiteralAST *minorAst = minorVersionExp->asNumericLiteral())
            minorLit = translationUnit()->numericLiteral(minorAst->literal_token);

        // build the descriptor
        ExportedQmlType exportedType;
        exportedType.isSingleton = registrationFunction == QmlRegisterSingletonTypeCallback1
                || registrationFunction == QmlRegisterSingletonTypeCallback2
                || registrationFunction == QmlRegisterSingletonTypeUrl;
        exportedType.isCreatable = !exportedType.isSingleton
                && registrationFunction != QmlRegisterUncreatableType;
        exportedType.typeName = QString::fromUtf8(nameLit->chars(), nameLit->size());
        exportedType.packageName = packageName;
        if (majorLit && minorLit && majorLit->isInt() && minorLit->isInt()) {
            exportedType.version = LanguageUtils::ComponentVersion(
                        QString::fromUtf8(majorLit->chars(), majorLit->size()).toInt(),
                        QString::fromUtf8(minorLit->chars(), minorLit->size()).toInt());
        }

        // we want to do lookup later, so also store the surrounding scope
        unsigned line, column;
        translationUnit()->getTokenStartPosition(ast->firstToken(), &line, &column);
        exportedType.scope = _doc->scopeAt(line, column);

        if (typeId){
            // add the expression
            const Token begin = translationUnit()->tokenAt(typeId->firstToken());
            const Token last = translationUnit()->tokenAt(typeId->lastToken() - 1);
            exportedType.typeExpression = QString::fromUtf8(
                        _doc->utf8Source().mid(begin.bytesBegin(), last.bytesEnd() - begin.bytesBegin()));
        } else if (registrationFunction == QmlRegisterSingletonTypeCallback2) {
            // we cannot really do much with generated QJSValue values
        } else if (registrationFunction == QmlRegisterSingletonTypeUrl
                   || registrationFunction == QmlRegisterType5) {
            // try to recover QUrl("literal")
            if (ast->expression_list && ast->expression_list->value) {
                ExpressionAST *urlAst = skipStringCall(skipQUrl(ast->expression_list->value, translationUnit()));
                if (StringLiteralAST *uriAst = urlAst->asStringLiteral()) {
                    const StringLiteral *urlLit = translationUnit()->stringLiteral(uriAst->literal_token);
                    exportedType.url = QString::fromUtf8(urlLit->chars(), urlLit->size());
                }
            }
        } else {
            qCWarning(QmlJS::qmljsLog()) << "missing template type for registrationFunction " << registrationFunction;
        }
        _exportedTypes += exportedType;

        return true;
    }

    static NameAST *callName(ExpressionAST *exp)
    {
        if (IdExpressionAST *idExp = exp->asIdExpression())
            return idExp->name;
        if (MemberAccessAST *memberExp = exp->asMemberAccess())
            return memberExp->member_name;
        return 0;
    }

    static ExpressionAST *skipQVariant(ExpressionAST *ast, TranslationUnit *translationUnit)
    {
        CallAST *call = ast->asCall();
        if (!call)
            return ast;
        if (!call->expression_list
                || !call->expression_list->value
                || call->expression_list->next)
            return ast;

        IdExpressionAST *idExp = call->base_expression->asIdExpression();
        if (!idExp || !idExp->name)
            return ast;

        // QVariant(foo) -> foo
        if (SimpleNameAST *simpleName = idExp->name->asSimpleName()) {
            const Identifier *id = translationUnit->identifier(simpleName->identifier_token);
            if (!id)
                return ast;
            if (QString::fromUtf8(id->chars(), id->size()) != QLatin1String("QVariant"))
                return ast;
            return call->expression_list->value;
        // QVariant::fromValue(foo) -> foo
        } else if (QualifiedNameAST *q = idExp->name->asQualifiedName()) {
            SimpleNameAST *simpleRhsName = q->unqualified_name->asSimpleName();
            if (!simpleRhsName
                    || !q->nested_name_specifier_list
                    || !q->nested_name_specifier_list->value
                    || q->nested_name_specifier_list->next)
                return ast;
            const Identifier *rhsId = translationUnit->identifier(simpleRhsName->identifier_token);
            if (!rhsId)
                return ast;
            if (QString::fromUtf8(rhsId->chars(), rhsId->size()) != QLatin1String("fromValue"))
                return ast;
            NestedNameSpecifierAST *nested = q->nested_name_specifier_list->value;
            SimpleNameAST *simpleLhsName = nested->class_or_namespace_name->asSimpleName();
            if (!simpleLhsName)
                return ast;
            const Identifier *lhsId = translationUnit->identifier(simpleLhsName->identifier_token);
            if (!lhsId)
                return ast;
            if (QString::fromUtf8(lhsId->chars(), lhsId->size()) != QLatin1String("QVariant"))
                return ast;
            return call->expression_list->value;
        }

        return ast;
    }

    static ExpressionAST *skipQUrl(ExpressionAST *ast, TranslationUnit *translationUnit)
    {
        CallAST *call = ast->asCall();
        if (!call)
            return ast;
        if (!call->expression_list
                || !call->expression_list->value
                || call->expression_list->next)
            return ast;

        IdExpressionAST *idExp = call->base_expression->asIdExpression();
        if (!idExp || !idExp->name)
            return ast;

        // QUrl(foo) -> foo
        if (SimpleNameAST *simpleName = idExp->name->asSimpleName()) {
            const Identifier *id = translationUnit->identifier(simpleName->identifier_token);
            if (!id)
                return ast;
            if (QByteArray(id->chars(), id->size()) == "QUrl")
                return call->expression_list->value;
        }

        return ast;
    }

    bool checkForSetContextProperty(CallAST *ast)
    {
        // check whether ast->base_expression has the 'setContextProperty' name
        NameAST *callNameAst = callName(ast->base_expression);
        if (!callNameAst)
            return false;
        SimpleNameAST *simpleNameAst = callNameAst->asSimpleName();
        if (!simpleNameAst || !simpleNameAst->identifier_token)
            return false;
        const Identifier *nameIdentifier = translationUnit()->identifier(simpleNameAst->identifier_token);
        if (!nameIdentifier)
            return false;
        const QString callName = QString::fromUtf8(nameIdentifier->chars(), nameIdentifier->size());
        if (callName != QLatin1String("setContextProperty"))
            return false;

        // must have two arguments
        if (!ast->expression_list
                || !ast->expression_list->value || !ast->expression_list->next
                || !ast->expression_list->next->value || ast->expression_list->next->next)
            return false;

        // first argument must be a string literal
        const StringLiteral *nameLit = 0;
        if (StringLiteralAST *nameAst = skipStringCall(ast->expression_list->value)->asStringLiteral())
            nameLit = translationUnit()->stringLiteral(nameAst->literal_token);
        if (!nameLit) {
            unsigned line, column;
            translationUnit()->getTokenStartPosition(ast->expression_list->value->firstToken(), &line, &column);
            _messages += Document::DiagnosticMessage(
                        Document::DiagnosticMessage::Warning,
                        _doc->fileName(),
                        line, column,
                        QmlJS::FindExportedCppTypes::tr(
                            "must be a string literal to be available in the QML editor"));
            return false;
        }

        ContextProperty contextProperty;
        contextProperty.name = QString::fromUtf8(nameLit->chars(), nameLit->size());
        contextProperty.expression = stringOf(skipQVariant(ast->expression_list->next->value, translationUnit()));
        // we want to do lookup later, so also store the line and column of the target scope
        translationUnit()->getTokenStartPosition(ast->firstToken(),
                                                 &contextProperty.line,
                                                 &contextProperty.column);

        _contextProperties += contextProperty;

        return true;
    }

private:
    QString stringOf(CPlusPlus::AST *ast)
    {
        return stringOf(ast->firstToken(), ast->lastToken() - 1);
    }

    QString stringOf(int first, int last)
    {
        const Token firstToken = translationUnit()->tokenAt(first);
        const Token lastToken = translationUnit()->tokenAt(last);
        return QString::fromUtf8(
            _doc->utf8Source().mid(firstToken.bytesBegin(),
                                   lastToken.bytesEnd() - firstToken.bytesBegin()));
    }

    QString stringOf(const Token &token)
    {
        return QString::fromUtf8(_doc->utf8Source().mid(token.bytesBegin(), token.bytes()));
    }

    ExpressionAST *skipStringCall(ExpressionAST *exp)
    {
        if (!exp || !exp->asCall())
            return exp;

        IdExpressionAST *callName = _builder.IdExpression();
        CallAST *call = _builder.Call(callName);
        if (!exp->match(call, &_matcher))
            return exp;

        const QString name = stringOf(callName);
        if (name != QLatin1String("QLatin1String")
                && name != QLatin1String("QString"))
            return exp;

        if (!call->expression_list || call->expression_list->next)
            return exp;

        return call->expression_list->value;
    }

    QString nameOfUriAssert(StatementAST *stmt, const QString &uriName)
    {
        QString null;

        IdExpressionAST *outerCallName = _builder.IdExpression();
        BinaryExpressionAST *binary = _builder.BinaryExpression();
        // assert(... == ...);
        ExpressionStatementAST *pattern = _builder.ExpressionStatement(
                    _builder.Call(outerCallName, _builder.ExpressionList(
                                     binary)));

        if (!stmt->match(pattern, &_matcher)) {
            outerCallName = _builder.IdExpression();
            binary = _builder.BinaryExpression();
            // the expansion of Q_ASSERT(...),
            // ((!(... == ...)) ? qt_assert(...) : ...);
            pattern = _builder.ExpressionStatement(
                        _builder.NestedExpression(
                            _builder.ConditionalExpression(
                                _builder.NestedExpression(
                                    _builder.UnaryExpression(
                                        _builder.NestedExpression(
                                            binary))),
                                _builder.Call(outerCallName))));

            if (!stmt->match(pattern, &_matcher))
                return null;
        }

        const QString outerCall = stringOf(outerCallName);
        if (outerCall != QLatin1String("qt_assert")
                && outerCall != QLatin1String("assert")
                && outerCall != QLatin1String("Q_ASSERT"))
            return null;

        if (translationUnit()->tokenAt(binary->binary_op_token).kind() != T_EQUAL_EQUAL)
            return null;

        ExpressionAST *lhsExp = skipStringCall(binary->left_expression);
        ExpressionAST *rhsExp = skipStringCall(binary->right_expression);
        if (!lhsExp || !rhsExp)
            return null;

        StringLiteralAST *uriString = lhsExp->asStringLiteral();
        IdExpressionAST *uriArgName = lhsExp->asIdExpression();
        if (!uriString)
            uriString = rhsExp->asStringLiteral();
        if (!uriArgName)
            uriArgName = rhsExp->asIdExpression();
        if (!uriString || !uriArgName)
            return null;

        if (stringOf(uriArgName) != uriName)
            return null;

        const StringLiteral *packageLit = translationUnit()->stringLiteral(uriString->literal_token);
        return QString::fromUtf8(packageLit->chars(), packageLit->size());
    }
};

static FullySpecifiedType stripPointerAndReference(const FullySpecifiedType &type)
{
    Type *t = type.type();
    while (t) {
        if (PointerType *ptr = t->asPointerType())
            t = ptr->elementType().type();
        else if (ReferenceType *ref = t->asReferenceType())
            t = ref->elementType().type();
        else
            break;
    }
    return FullySpecifiedType(t);
}

static QString toQmlType(const FullySpecifiedType &type)
{
    Overview overview;
    QString result = overview.prettyType(stripPointerAndReference(type));
    if (result == QLatin1String("QString"))
        result = QLatin1String("string");
    return result;
}

static Class *lookupClass(const QString &expression, Scope *scope, TypeOfExpression &typeOf)
{
    QList<LookupItem> results = typeOf(expression.toUtf8(), scope);
    Class *klass = 0;
    foreach (const LookupItem &item, results) {
        if (item.declaration()) {
            klass = item.declaration()->asClass();
            if (klass)
                return klass;
        }
    }
    return 0;
}

static LanguageUtils::FakeMetaObject::Ptr buildFakeMetaObject(
        Class *klass,
        QHash<Class *, LanguageUtils::FakeMetaObject::Ptr> *fakeMetaObjects,
        TypeOfExpression &typeOf)
{
    using namespace LanguageUtils;

    if (FakeMetaObject::Ptr fmo = fakeMetaObjects->value(klass))
        return fmo;

    FakeMetaObject::Ptr fmo(new FakeMetaObject);
    if (!klass)
        return fmo;
    fakeMetaObjects->insert(klass, fmo);

    Overview namePrinter;

    fmo->setClassName(namePrinter.prettyName(klass->name()));
    // add the no-package export, so the cpp name can be used in properties
    fmo->addExport(fmo->className(), QmlJS::CppQmlTypes::cppPackage, ComponentVersion());

    for (unsigned i = 0; i < klass->memberCount(); ++i) {
        Symbol *member = klass->memberAt(i);
        if (!member->name())
            continue;
        if (Function *func = member->type()->asFunctionType()) {
            if (!func->isSlot() && !func->isInvokable() && !func->isSignal())
                continue;
            FakeMetaMethod method(namePrinter.prettyName(func->name()), toQmlType(func->returnType()));
            if (func->isSignal())
                method.setMethodType(FakeMetaMethod::Signal);
            else
                method.setMethodType(FakeMetaMethod::Slot);
            for (unsigned a = 0, argc = func->argumentCount(); a < argc; ++a) {
                Symbol *arg = func->argumentAt(a);
                QString name;
                if (arg->name())
                    name = namePrinter.prettyName(arg->name());
                method.addParameter(name, toQmlType(arg->type()));
            }
            fmo->addMethod(method);
        }
        if (QtPropertyDeclaration *propDecl = member->asQtPropertyDeclaration()) {
            const FullySpecifiedType &type = propDecl->type();
            const bool isList = false; // ### fixme
            const bool isWritable = propDecl->flags() & QtPropertyDeclaration::WriteFunction;
            const bool isPointer = type.type() && type.type()->isPointerType();
            const int revision = 0; // ### fixme
            FakeMetaProperty property(
                        namePrinter.prettyName(propDecl->name()),
                        toQmlType(type),
                        isList, isWritable, isPointer,
                        revision);
            fmo->addProperty(property);
        }
        if (QtEnum *qtEnum = member->asQtEnum()) {
            // find the matching enum
            Enum *e = 0;
            QList<LookupItem> result = typeOf(namePrinter.prettyName(qtEnum->name()).toUtf8(), klass);
            foreach (const LookupItem &item, result) {
                if (item.declaration()) {
                    e = item.declaration()->asEnum();
                    if (e)
                        break;
                }
            }
            if (!e)
                continue;

            FakeMetaEnum metaEnum(namePrinter.prettyName(e->name()));
            for (unsigned j = 0; j < e->memberCount(); ++j) {
                Symbol *enumMember = e->memberAt(j);
                if (!enumMember->name())
                    continue;
                metaEnum.addKey(namePrinter.prettyName(enumMember->name()), 0);
            }
            fmo->addEnum(metaEnum);
        }
    }

    // only single inheritance is supported
    if (klass->baseClassCount() > 0) {
        BaseClass *base = klass->baseClassAt(0);
        if (!base->name())
            return fmo;
        const QString baseClassName = namePrinter.prettyName(base->name());
        fmo->setSuperclassName(baseClassName);

        Class *baseClass = lookupClass(baseClassName, klass, typeOf);
        if (!baseClass)
            return fmo;
        buildFakeMetaObject(baseClass, fakeMetaObjects, typeOf);
    }

    return fmo;
}

static void buildExportedQmlObjects(
        TypeOfExpression &typeOf,
        const QList<ExportedQmlType> &cppExports,
        QHash<Class *, LanguageUtils::FakeMetaObject::Ptr> *fakeMetaObjects,
        QList<LanguageUtils::FakeMetaObject::Ptr> *extraFakeMetaObjects)
{
    using namespace LanguageUtils;

    if (cppExports.isEmpty())
        return;

    foreach (const ExportedQmlType &exportedType, cppExports) {
        Class *klass = 0;
        if (!exportedType.typeExpression.isEmpty())
            klass = lookupClass(exportedType.typeExpression, exportedType.scope, typeOf);
        // TODO: something smarter with exportedType.url
        // accepts a null klass
        FakeMetaObject::Ptr fmo = buildFakeMetaObject(klass, fakeMetaObjects, typeOf);
        fmo->addExport(exportedType.typeName,
                       exportedType.packageName,
                       exportedType.version);
        fmo->setIsCreatable(exportedType.isCreatable);
        fmo->setIsSingleton(exportedType.isSingleton);
        if (!klass) {
            fmo->setClassName(QLatin1String("__autogen__") + exportedType.typeName);
            extraFakeMetaObjects->append(fmo);
        }
    }
}

static void buildContextProperties(
        const Document::Ptr &doc,
        TypeOfExpression &typeOf,
        const QList<ContextProperty> &contextPropertyDescriptions,
        QHash<Class *, LanguageUtils::FakeMetaObject::Ptr> *fakeMetaObjects,
        QHash<QString, QString> *contextProperties)
{
    using namespace LanguageUtils;

    foreach (const ContextProperty &property, contextPropertyDescriptions) {
        Scope *scope = doc->scopeAt(property.line, property.column);
        QList<LookupItem> results = typeOf(property.expression.toUtf8(), scope);
        QString typeName;
        if (!results.isEmpty()) {
            LookupItem result = results.first();
            FullySpecifiedType simpleType = stripPointerAndReference(result.type());
            if (NamedType *namedType = simpleType.type()->asNamedType()) {
                Scope *typeScope = result.scope();
                if (!typeScope)
                    typeScope = scope; // incorrect but may be an ok fallback
                ClassOrNamespace *binding = typeOf.context().lookupType(namedType->name(), typeScope);
                if (binding && !binding->symbols().isEmpty()) {
                    // find the best 'Class' symbol
                    for (int i = binding->symbols().size() - 1; i >= 0; --i) {
                        if (Class *klass = binding->symbols().at(i)->asClass()) {
                            FakeMetaObject::Ptr fmo = buildFakeMetaObject(klass, fakeMetaObjects, typeOf);
                            typeName = fmo->className();
                            break;
                        }
                    }
                }
            }
        }

        contextProperties->insert(property.name, typeName);
    }
}

} // anonymous namespace

namespace QmlJS {

FindExportedCppTypes::FindExportedCppTypes(const CPlusPlus::Snapshot &snapshot)
    : m_snapshot(snapshot)
{
}

QStringList FindExportedCppTypes::operator()(const CPlusPlus::Document::Ptr &document)
{
    QTC_ASSERT(!document.isNull(), return QStringList());

    m_contextProperties.clear();
    m_exportedTypes.clear();
    QStringList fileNames;

    // this check only guards against some input errors, if document's source and AST has not
    // been guarded properly the source and AST may still become empty/null while this function is running
    if (document->utf8Source().isEmpty()
            || !document->translationUnit()->ast())
        return fileNames;

    FindExportsVisitor finder(document);
    finder();
    static const QString kindKey = QLatin1String("QmlJSTools.ExportedQmlTypesDiagnostic");
    CppModelManagerBase::trySetExtraDiagnostics(document->fileName(), kindKey,
                                                finder.messages());

    // if nothing was found, done
    const QList<ContextProperty> contextPropertyDescriptions = finder.contextProperties();
    const QList<ExportedQmlType> exports = finder.exportedTypes();
    if (exports.isEmpty() && contextPropertyDescriptions.isEmpty())
        return fileNames;

    // context properties need lookup inside function scope, and thus require a full check
    CPlusPlus::Document::Ptr localDoc = document;
    if (document->checkMode() != CPlusPlus::Document::FullCheck && !contextPropertyDescriptions.isEmpty()) {
        localDoc = m_snapshot.documentFromSource(document->utf8Source(), document->fileName());
        localDoc->check();
    }

    // create a single type of expression (and bindings) for the document
    TypeOfExpression typeOf;
    typeOf.init(localDoc, m_snapshot);
    QHash<Class *, LanguageUtils::FakeMetaObject::Ptr> fakeMetaObjects;
    QList<LanguageUtils::FakeMetaObject::Ptr> extraFakeMetaObjects;

    // generate the exports from qmlRegisterType
    buildExportedQmlObjects(typeOf, exports, &fakeMetaObjects, &extraFakeMetaObjects);

    // add the types from the context properties and create a name->cppname map
    // also expose types where necessary
    buildContextProperties(localDoc, typeOf, contextPropertyDescriptions,
                           &fakeMetaObjects, &m_contextProperties);

    // convert to list of FakeMetaObject::ConstPtr
    m_exportedTypes.reserve(fakeMetaObjects.size() + extraFakeMetaObjects.size());
    fileNames.reserve(fakeMetaObjects.size());
    for (auto it = fakeMetaObjects.constBegin(), end = fakeMetaObjects.constEnd(); it != end;
         ++it) {
        it.value()->updateFingerprint();
        m_exportedTypes += it.value();
        fileNames += QLatin1String(it.key()->fileName());
    }
    foreach (const LanguageUtils::FakeMetaObject::Ptr &fmo, extraFakeMetaObjects) {
        fmo->updateFingerprint();
        m_exportedTypes += fmo;
    }

    return fileNames;
}

QList<LanguageUtils::FakeMetaObject::ConstPtr> FindExportedCppTypes::exportedTypes() const
{
    return m_exportedTypes;
}

QHash<QString, QString> FindExportedCppTypes::contextProperties() const
{
    return m_contextProperties;
}

bool FindExportedCppTypes::maybeExportsTypes(const CPlusPlus::Document::Ptr &document)
{
    if (!document->control())
        return false;
    const QByteArray qmlRegisterSingletonTypeToken("qmlRegisterType");
    const QByteArray qmlRegisterTypeToken("qmlRegisterSingletonType");
    const QByteArray qmlRegisterUncreatableTypeToken("qmlRegisterUncreatableType");
    const QByteArray setContextPropertyToken("setContextProperty");
    if (document->control()->findIdentifier(
                qmlRegisterTypeToken.constData(), qmlRegisterTypeToken.size())) {
        return true;
    }
    if (document->control()->findIdentifier(
                qmlRegisterSingletonTypeToken.constData(), qmlRegisterSingletonTypeToken.size())) {
        return true;
    }
    if (document->control()->findIdentifier(
                qmlRegisterUncreatableTypeToken.constData(), qmlRegisterUncreatableTypeToken.size())) {
        return true;
    }
    if (document->control()->findIdentifier(
                setContextPropertyToken.constData(), setContextPropertyToken.size())) {
        return true;
    }
    return false;
}

} // namespace QmlJS
