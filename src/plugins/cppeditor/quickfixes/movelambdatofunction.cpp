// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "movelambdatofunction.h"

#include "../cppcodestylesettings.h"
#include "../cppeditortr.h"
#include "../cpprefactoringchanges.h"
#include "../cpptoolsreuse.h"
#include "../insertionpointlocator.h"
#include "cppquickfix.h"
#include "cppquickfixhelpers.h"

#include <coreplugin/icore.h>
#include <cplusplus/ASTVisitor.h>
#include <cplusplus/CppRewriter.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Token.h>
#include <utils/qtcassert.h>

#include <QHash>
#include <QInputDialog>
#include <QLineEdit>

#include <functional>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#endif

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

using FunctionNameGetter = std::function<QString()>;

enum class LambdaCapture { None, This };
enum class MoveLambdaTarget { FreeFunction, MemberFunction, Slot };

// disconnect() has no overload taking a functor, so a lambda passed to it could
// never have compiled in the first place; only connect() is relevant here.
// Deliberately limited to an unqualified or class-qualified callee ("connect(...)",
// "QObject::connect(...)"): a member-access callee ("obj->connect(...)") would need
// resolving obj's type to find out whether it inherits QObject, rather than the
// simple scope lookup isQObjectMember() below does, and connect() being called
// through some other object's pointer is already excluded further down this file
// for the four-argument form (the context object must be "this").
bool isConnectCall(CallAST *call, const Name **name)
{
    if (!call || !call->base_expression)
        return false;
    const IdExpressionAST * const idExpr = call->base_expression->asIdExpression();
    if (!idExpr || !idExpr->name || !idExpr->name->name)
        return false;
    const Identifier * const id = idExpr->name->name->identifier();
    if (!id)
        return false;
    const QString text = Overview{}.prettyName(id);
    if (text != "connect")
        return false;
    *name = idExpr->name->name;
    return true;
}

// "connect" is a common name; make sure it actually refers to a
// member inherited from QObject, the way CppModelManager::getSignalSlotType() does.
bool isQObjectMember(const Name *name, Scope *scope, const LookupContext &context)
{
    const QList<LookupItem> matches = context.lookup(name, scope);
    for (const LookupItem &match : matches) {
        if (!match.scope())
            continue;
        const Class * const klass = match.scope()->asClass();
        if (!klass || !klass->name())
            continue;
        if (Overview{}.prettyName(klass->name()) == "QObject")
            return true;
    }
    return false;
}

bool hasAutoParameter(LambdaDeclaratorAST *declarator, const CppRefactoringFilePtr &file)
{
    if (!declarator || !declarator->parameter_declaration_clause)
        return false;
    for (ParameterDeclarationListAST *it
         = declarator->parameter_declaration_clause->parameter_declaration_list;
         it; it = it->next) {
        ParameterDeclarationAST * const param = it->value;
        if (!param)
            continue;
        for (SpecifierListAST *specIt = param->type_specifier_list; specIt; specIt = specIt->next) {
            if (SimpleSpecifierAST * const simpleSpec = specIt->value->asSimpleSpecifier()) {
                if (file->tokenAt(simpleSpec->specifier_token).kind() == T_AUTO)
                    return true;
            }
        }
    }
    return false;
}

// Does not descend into nested lambdas or local function definitions.
class ReturnStatementFinder : public ASTVisitor
{
public:
    explicit ReturnStatementFinder(TranslationUnit *unit) : ASTVisitor(unit) {}

    QList<ReturnStatementAST *> operator()(StatementAST *body)
    {
        accept(body);
        return m_result;
    }

private:
    bool preVisit(AST *ast) override
    {
        return !ast->asLambdaExpression() && !ast->asFunctionDefinition();
    }

    bool visit(ReturnStatementAST *ast) override
    {
        m_result << ast;
        return false;
    }

    QList<ReturnStatementAST *> m_result;
};

// An invalid return value means the return type could not be determined (typeOfExpr() failed
// on a non-empty return expression, e.g. because it depends on a template parameter, or because
// two return statements disagree - the only way that can happen in a lambda that compiles at all
// is a discarded "if constexpr" branch, since the language otherwise requires every return
// statement to deduce the exact same type) - a genuinely void lambda (no return statement, or a
// return with no expression) never reaches typeOfExpr() and so always succeeds.
FullySpecifiedType computeReturnType(
    const CppQuickFixInterface &interface, LambdaExpressionAST *lambda)
{
    LambdaDeclaratorAST * const declarator = lambda->lambda_declarator;
    if (declarator && declarator->trailing_return_type && declarator->symbol)
        return declarator->symbol->returnType();

    ReturnStatementFinder finder(interface.semanticInfo().doc->translationUnit());
    const QList<ReturnStatementAST *> returnStmts = finder(lambda->statement);
    if (returnStmts.isEmpty())
        return FullySpecifiedType(&VoidType::instance);

    FullySpecifiedType result;
    for (ReturnStatementAST * const returnStmt : returnStmts) {
        const FullySpecifiedType type = returnStmt->expression
            ? typeOfExpr(returnStmt->expression, interface.currentFile(), interface.snapshot(),
                         interface.context())
            : FullySpecifiedType(&VoidType::instance);
        if (!type.isValid())
            return {};
        if (!result.isValid())
            result = type;
        else if (!result.match(type))
            return {};
    }
    return result;
}

// Puts the opening brace, the body, and the closing brace on their own lines. Lambda
// bodies are idiomatically written on a single line ("{ return x; }"), but the same is not
// true for named function; the surrounding refactoring machinery does not necessarily
// do a full re-formatting of the result, so insert some newlines if necessary.
QString normalizedBody(const QString &compoundStatementText)
{
    const QString inner = compoundStatementText.mid(1, compoundStatementText.size() - 2).trimmed();
    return inner.isEmpty() ? QString("{\n}") : "{\n" + inner + "\n}";
}

// If function is a namespace-qualified out-of-line definition (e.g. "void ns::foo()"
// written outside "namespace ns { ... }"), returns the namespace chain (outermost first)
// that needs to be reproduced around a new sibling definition. Returns an empty list for
// an unqualified, already-nested definition, which needs no wrapping. Note that a
// function's own enclosingScope() cannot be used to decide this: Bind::visit(
// FunctionDefinitionAST*) adds the symbol to whatever scope is lexically current, which for
// a qualified definition is wherever it was written (often the global namespace), not the
// namespace(s) named by the qualifier. LookupContext::fullyQualifiedName() already accounts
// for that (see its handling of Symbol::asFunction()), so it can be reused here directly.
QStringList missingNamespaces(Function *function)
{
    if (!function->name() || !function->name()->asQualifiedNameId())
        return {};

    QList<const Name *> names = LookupContext::fullyQualifiedName(function);
    if (names.size() <= 1)
        return {};
    names.removeLast(); // the function's own name

    QStringList result;
    for (const Name *n : std::as_const(names))
        result << Overview{}.prettyName(n);
    return result;
}

class MoveLambdaToFunctionOp : public CppQuickFixOperation
{
public:
    MoveLambdaToFunctionOp(
        const CppQuickFixInterface &interface,
        LambdaExpressionAST *lambda,
        int lambdaArgPos,
        FunctionDefinitionAST *refFuncDef,
        Class *matchingClass,
        MoveLambdaTarget target,
        const FullySpecifiedType &returnType,
        const QStringList &missingNamespaces,
        FunctionNameGetter nameGetter = {})
        : CppQuickFixOperation(interface)
        , m_lambda(lambda)
        , m_lambdaArgPos(lambdaArgPos)
        , m_refFuncDef(refFuncDef)
        , m_matchingClass(matchingClass)
        , m_target(target)
        , m_returnType(returnType)
        , m_missingNamespaces(missingNamespaces)
        , m_nameGetter(nameGetter)
    {
        QTC_ASSERT(matchingClass || target == MoveLambdaTarget::FreeFunction, return);
        switch (m_target) {
        case MoveLambdaTarget::FreeFunction:
            setDescription(Tr::tr("Move to Function"));
            break;
        case MoveLambdaTarget::MemberFunction:
            setDescription(Tr::tr("Move to Member Function"));
            break;
        case MoveLambdaTarget::Slot:
            setDescription(Tr::tr("Move to Slot"));
            break;
        }
    }

private:
    void perform() override
    {
        const QString funcName = getFunctionName();
        if (funcName.isEmpty())
            return;

        LambdaDeclaratorAST * const declarator = m_lambda->lambda_declarator;
        const QString paramsText = declarator && declarator->parameter_declaration_clause
                                        ? currentFile()->textOf(
                                              declarator->parameter_declaration_clause)
                                        : QString();
        const QString trailingSpecifiers = lambdaTrailingSpecifiers(declarator);
        const QString bodyText = normalizedBody(currentFile()->textOf(m_lambda->statement));

        if (!m_matchingClass)
            performFreeFunction(funcName, paramsText, trailingSpecifiers, bodyText);
        else
            performMember(funcName, paramsText, trailingSpecifiers, bodyText);
    }

    // The lambda's own exception-specification and attribute-specifiers, exactly as
    // written (e.g. " noexcept(false) [[nodiscard]]") - dropping them would change
    // what connect() ends up calling, since a noexcept-specification is part of a
    // function's type as of C++17. Exception-specification comes before attributes
    // here because that is the order an ordinary function declarator requires
    // ("parameters-and-qualifiers" in the grammar), which differs from the lambda's
    // own declarator, where the attributes are written first. "mutable" is
    // deliberately excluded: it has no equivalent for the ordinary function or
    // member function being created.
    QString lambdaTrailingSpecifiers(LambdaDeclaratorAST *declarator) const
    {
        if (!declarator)
            return {};
        QString result;
        if (declarator->exception_specification)
            result += ' ' + currentFile()->textOf(declarator->exception_specification);
        for (SpecifierListAST *it = declarator->attributes; it; it = it->next)
            result += ' ' + currentFile()->textOf(it->value);
        return result;
    }

    QString getFunctionName() const
    {
        if (m_nameGetter)
            return m_nameGetter();

        Scope * const targetScope = m_matchingClass
                                         ? static_cast<Scope *>(m_matchingClass)
                                         : static_cast<Scope *>(
                                               isNamespaceFunction(context(), m_refFuncDef->symbol));
        Control * const control = context().bindings()->control().get();
        QString lastAttempt;
        QString label = Tr::tr("Function name:");
        while (true) {
            bool ok = false;
            const QString name = QInputDialog::getText(
                Core::ICore::dialogParent(),
                Tr::tr("Move Lambda"),
                label,
                QLineEdit::Normal,
                lastAttempt,
                &ok);
            if (!ok)
                return {};
            lastAttempt = name;
            if (!isValidIdentifier(name)) {
                label = Tr::tr("\"%1\" is not a valid identifier. Function name:").arg(name);
                continue;
            }
            if (targetScope) {
                // A courtesy check only: it looks at targetScope alone, so a clash with an
                // inherited member or a name from an enclosing namespace still gets through.
                const QByteArray utf8Name = name.toUtf8();
                const Identifier * const id
                    = control->identifier(utf8Name.constData(), utf8Name.size());
                if (targetScope->find(id)) {
                    label = Tr::tr("\"%1\" is already in use here. Function name:").arg(name);
                    continue;
                }
            }
            return name;
        }
    }

    QString plainReturnType() const
    {
        return CppCodeStyleSettings::currentProjectCodeStyleOverview().prettyType(m_returnType);
    }

    void performFreeFunction(const QString &funcName, const QString &paramsText,
                             const QString &trailingSpecifiers, const QString &bodyText)
    {
        QString defText = inlinePrefix(filePath()) + plainReturnType() + ' ' + funcName + '('
                           + paramsText + ')' + trailingSpecifiers + "\n" + bodyText + "\n\n";
        QString namespacePrefix;
        QString namespaceSuffix;
        for (const QString &ns : m_missingNamespaces) {
            namespacePrefix += "namespace " + ns + " {\n";
            namespaceSuffix += "}\n";
        }
        defText = namespacePrefix + defText + namespaceSuffix;

        ChangeSet change;
        change.insert(currentFile()->startOf(m_refFuncDef), defText);
        change.replace(currentFile()->range(m_lambda), funcName);
        currentFile()->apply(change);
    }

    ClassOrNamespace *targetClassOrNamespace() const
    {
        ClassOrNamespace *targetCoN = context().lookupType(m_refFuncDef->symbol->enclosingScope());
        return targetCoN ? targetCoN : context().globalNamespace();
    }

    QString classQualification() const
    {
        Control * const control = context().bindings()->control().get();
        const Name * const minimalName = LookupContext::minimalName(
            m_matchingClass, targetClassOrNamespace(), control);
        return CppCodeStyleSettings::currentProjectCodeStyleOverview().prettyName(minimalName)
               + "::";
    }

    // The leading return type of an out-of-class member function definition does not get
    // the class's scope for lookup (unlike the parameter list, which comes after the
    // qualified name and does), so it needs to be qualified explicitly if necessary.
    QString qualifiedReturnType() const
    {
        SubstitutionEnvironment env;
        env.setContext(context());
        env.switchScope(m_refFuncDef->symbol);
        UseMinimalNames subs(targetClassOrNamespace());
        env.enter(&subs);
        Control * const control = context().bindings()->control().get();
        const FullySpecifiedType rewritten = rewriteType(m_returnType, &env, control);
        return CppCodeStyleSettings::currentProjectCodeStyleOverview().prettyType(rewritten);
    }

    void performMember(const QString &funcName, const QString &paramsText,
                       const QString &trailingSpecifiers, const QString &bodyText)
    {
        CppRefactoringChanges refactoring(snapshot());
        const QString classQual = classQualification();

        const InsertionPointLocator::AccessSpec accessSpec = m_target == MoveLambdaTarget::Slot
                                                                  ? InsertionPointLocator::PrivateSlot
                                                                  : InsertionPointLocator::Private;
        const QString declText = plainReturnType() + ' ' + funcName + '(' + paramsText + ')'
                                  + trailingSpecifiers + ";";

        const FilePath classFilePath = m_matchingClass->filePath();
        bool classIsInHeader = false;
        const FilePath cppFilePath = correspondingHeaderOrSource(classFilePath, &classIsInHeader);
        const FilePath defTargetFilePath = classIsInHeader && cppFilePath.exists()
                                               ? cppFilePath
                                               : classFilePath;

        QStringList insertedNamespaces;
        const InsertionLocation defLoc = insertLocationForMethodDefinition(
            m_matchingClass,
            false,
            NamespaceHandling::CreateMissing,
            refactoring,
            defTargetFilePath,
            &insertedNamespaces);

        const QString defText = inlinePrefix(defTargetFilePath) + qualifiedReturnType() + ' '
                                 + classQual + funcName + '(' + paramsText + ')'
                                 + trailingSpecifiers + "\n" + bodyText;

        QString replacement = QLatin1Char('&') + classQual + funcName;
        if (m_lambdaArgPos == 3)
            replacement.prepend("this, ");

        QHash<FilePath, ChangeSet> changes;

        const InsertionPointLocator locator(refactoring);
        const InsertionLocation declLoc = locator.methodDeclarationInClass(
            classFilePath, m_matchingClass, accessSpec);
        const CppRefactoringFilePtr declFile = refactoring.cppFile(classFilePath);
        changes[classFilePath].insert(
            declFile->position(declLoc.line(), declLoc.column()),
            declLoc.prefix() + declText + declLoc.suffix());

        const CppRefactoringFilePtr defFile = refactoring.cppFile(defTargetFilePath);
        changes[defTargetFilePath].insert(
            defFile->position(defLoc.line(), defLoc.column()),
            defLoc.prefix() + defText + defLoc.suffix());

        changes[filePath()].replace(currentFile()->range(m_lambda), replacement);

        for (auto it = changes.cbegin(); it != changes.cend(); ++it)
            refactoring.cppFile(it.key())->apply(it.value());
    }

    LambdaExpressionAST * const m_lambda;
    const int m_lambdaArgPos;
    FunctionDefinitionAST * const m_refFuncDef;
    Class * const m_matchingClass;
    const MoveLambdaTarget m_target;
    const FullySpecifiedType m_returnType;
    const QStringList m_missingNamespaces;
    const FunctionNameGetter m_nameGetter;
};

//! Turns a lambda passed to connect() into a named function.
class MoveLambdaToFunction : public CppQuickFixFactory
{
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        const CppRefactoringFilePtr file = interface.currentFile();

        int lambdaIndex = -1;
        LambdaExpressionAST *lambda = nullptr;
        // i > 0: path.at(lambdaIndex - 1) below must stay in bounds, and index 0 could
        // never be a connect() argument anyway (nothing encloses it).
        for (int i = path.size() - 1; i > 0; --i) {
            lambda = path.at(i)->asLambdaExpression();
            if (lambda) {
                lambdaIndex = i;
                break;
            }
        }
        if (!lambda || !lambda->statement || lambda->templateParameters || lambda->requiresClause)
            return;

        LambdaDeclaratorAST * const declarator = lambda->lambda_declarator;
        if (declarator) {
            if (declarator->requiresClause)
                return;
            if (hasAutoParameter(declarator, file))
                return;
        }

        LambdaIntroducerAST * const introducer = lambda->lambda_introducer;
        QTC_ASSERT(introducer, return);
        LambdaCapture capture = LambdaCapture::None;
        if (introducer->lambda_capture) {
            if (introducer->lambda_capture->default_capture_token)
                return;
            CaptureListAST * const captureList = introducer->lambda_capture->capture_list;
            if (!captureList)
                return;

            // Parser::parseCapture() leaves the CaptureAST null only for "this" (every
            // other capture gets a real one), so a single null entry means "[this]".
            if (captureList->next || captureList->value)
                return; // more than one capture, or a capture other than "this"
            capture = LambdaCapture::This;
        }

        CallAST * const connectCall = path.at(lambdaIndex - 1)->asCall();
        const Name *connectName = nullptr;
        if (!isConnectCall(connectCall, &connectName))
            return;

        int lambdaArgPos = 0;
        int argIndex = 0;
        for (ExpressionListAST *it = connectCall->expression_list; it; it = it->next) {
            ++argIndex;
            if (it->value == lambda) {
                lambdaArgPos = argIndex;
                break;
            }
        }
        if (lambdaArgPos != 3 && lambdaArgPos != 4)
            return;

        FunctionDefinitionAST *refFuncDef = nullptr;
        for (int i = lambdaIndex - 1; !refFuncDef && i >= 0; --i)
            refFuncDef = path.at(i)->asFunctionDefinition();
        if (!refFuncDef || !refFuncDef->symbol || !refFuncDef->symbol->name())
            return;

        // The lambda body can depend on a template parameter of refFuncDef itself, or of
        // its enclosing class, even when its return type does not; the moved-out function
        // (or member function) would not be a template and so would not compile.
        // enclosingTemplate() walks the scope chain, so this also catches a member function
        // of a template class.
        if (refFuncDef->symbol->enclosingTemplate())
            return;

        if (!isQObjectMember(connectName, refFuncDef->symbol, interface.context()))
            return;

        // isMemberFunction() only recognizes an out-of-line, qualified-name definition
        // ("void Foo::setup()"); an in-class inline one ("void setup() { ... }" written
        // inside "struct Foo") has an unqualified name and so is not covered by it, but its
        // enclosing scope is directly the class, which is simpler to check here than to
        // fold into that shared helper (whose other callers rely on the qualified name it
        // requires).
        Class *matchingClass = isMemberFunction(interface.context(), refFuncDef->symbol);
        if (!matchingClass)
            matchingClass = refFuncDef->symbol->enclosingScope()->asClass();
        if (!matchingClass && capture != LambdaCapture::None)
            return;

        // The four-argument form keeps its context object, which becomes the receiver
        // of "&Class::funcName". That pointer-to-member is only usable if the context
        // object is "this": any other object need not be of matchingClass's type (or a
        // subclass of it), e.g. connect(x, &X::sig, someWidget, [this] { ... }).
        if (matchingClass && lambdaArgPos == 4) {
            ExpressionAST *contextArg = nullptr;
            int contextArgIndex = 0;
            for (ExpressionListAST *it = connectCall->expression_list; it; it = it->next) {
                ++contextArgIndex;
                if (contextArgIndex == 3) {
                    contextArg = it->value;
                    break;
                }
            }
            if (!contextArg || !contextArg->asThisExpression())
                return;
        }

        const FullySpecifiedType returnType = computeReturnType(interface, lambda);
        if (!returnType.isValid())
            return;

        FunctionNameGetter nameGetter;
        if (testMode())
            nameGetter = [] { return QLatin1String("movedLambda"); };

        if (!matchingClass) {
            result << new MoveLambdaToFunctionOp(
                interface, lambda, lambdaArgPos, refFuncDef, nullptr, MoveLambdaTarget::FreeFunction,
                returnType, missingNamespaces(refFuncDef->symbol), nameGetter);
            return;
        }

        result << new MoveLambdaToFunctionOp(
            interface, lambda, lambdaArgPos, refFuncDef, matchingClass,
            MoveLambdaTarget::MemberFunction, returnType, {}, nameGetter);

        // "Move to Slot" only makes sense if matchingClass itself has a meta-object, i.e. is
        // a QObject subclass.
        // Note that we do not check for the presence of Q_OBJECT; the user will have to add
        // that themselves if necessary.
        if (isQObjectMember(connectName->identifier(), matchingClass, interface.context())) {
            result << new MoveLambdaToFunctionOp(
                interface, lambda, lambdaArgPos, refFuncDef, matchingClass,
                MoveLambdaTarget::Slot, returnType, {}, nameGetter);
        }
    }
};

#ifdef WITH_TESTS
class MoveLambdaToFunctionTest : public Tests::CppQuickFixTestObject
{
    Q_OBJECT
public:
    using CppQuickFixTestObject::CppQuickFixTestObject;
};
#endif

} // namespace

void registerMoveLambdaQuickfixes()
{
    REGISTER_QUICKFIX_FACTORY_WITH_STANDARD_TEST(MoveLambdaToFunction);
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <movelambdatofunction.moc>
#endif
