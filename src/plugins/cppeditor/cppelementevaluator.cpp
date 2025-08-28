// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppelementevaluator.h"

#include "cppmodelmanager.h"
#include "cpptoolsreuse.h"
#include "symbolfinder.h"
#include "typehierarchybuilder.h"

#include <texteditor/textdocument.h>

#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Icons.h>
#include <cplusplus/TypeOfExpression.h>

#include <utils/async.h>

#include <QDir>
#include <QQueue>
#include <QSet>

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {

struct SourceData
{
    Document::Ptr doc;
    Scope *scope;
    QString expression;
};
using SourceFunction = std::function<std::optional<SourceData>(const CPlusPlus::Snapshot &)>;

struct ExecData
{
    CPlusPlus::Snapshot snapshot;
    CPlusPlus::LookupItem lookupItem;
    CPlusPlus::LookupContext context;
};
using ExecFunction = std::function<QFuture<std::shared_ptr<CppElement>>(const ExecData &)>;

static QStringList stripName(const QString &name)
{
    QStringList all;
    all << name;
    int colonColon = 0;
    const int size = name.size();
    while ((colonColon = name.indexOf(QLatin1String("::"), colonColon)) != -1) {
        all << name.right(size - colonColon - 2);
        colonColon += 2;
    }
    return all;
}

CppElement::CppElement() = default;

CppElement::~CppElement() = default;

CppClass *CppElement::toCppClass()
{
    return nullptr;
}

class Unknown : public CppElement
{
public:
    explicit Unknown(const QString &type) : type(type)
    {
        tooltip = type;
    }

public:
    QString type;
};

class CppInclude : public CppElement
{
public:
    explicit CppInclude(const Document::Include &includeFile)
        : path(includeFile.resolvedFileName())
        , fileName(path.fileName())
    {
        helpCategory = Core::HelpItem::Brief;
        helpIdCandidates = QStringList(fileName);
        helpMark = fileName;
        link = Utils::Link(path);
        tooltip = path.toUserOutput();
    }

public:
    FilePath path;
    QString fileName;
};

class CppMacro : public CppElement
{
public:
    explicit CppMacro(const Macro &macro)
    {
        helpCategory = Core::HelpItem::Macro;
        const QString macroName = QString::fromUtf8(macro.name(), macro.name().size());
        helpIdCandidates = QStringList(macroName);
        helpMark = macroName;
        link = Link(macro.filePath(), macro.line());
        tooltip = macro.toStringWithLineBreaks();
    }
};

// CppDeclarableElement
CppDeclarableElement::CppDeclarableElement(Symbol *declaration)
    : CppElement()
    , iconType(CPlusPlus::Icons::iconTypeForSymbol(declaration))
{
    Overview overview;
    overview.showArgumentNames = true;
    overview.showReturnTypes = true;
    overview.showTemplateParameters = true;
    name = overview.prettyName(declaration->name());
    if (declaration->enclosingScope()->asClass() ||
        declaration->enclosingScope()->asNamespace() ||
        declaration->enclosingScope()->asEnum() ||
        declaration->enclosingScope()->asTemplate()) {
        qualifiedName = overview.prettyName(LookupContext::fullyQualifiedName(declaration));
        helpIdCandidates = stripName(qualifiedName);
    } else {
        qualifiedName = name;
        helpIdCandidates.append(name);
    }

    tooltip = overview.prettyType(declaration->type(), qualifiedName);
    link = declaration->toLink();
    helpMark = name;
}

class CppNamespace : public CppDeclarableElement
{
public:
    explicit CppNamespace(Symbol *declaration)
        : CppDeclarableElement(declaration)
    {
        helpCategory = Core::HelpItem::ClassOrNamespace;
        tooltip = qualifiedName;
    }
};

CppClass::CppClass(Symbol *declaration) : CppDeclarableElement(declaration)
{
    helpCategory = Core::HelpItem::ClassOrNamespace;
    tooltip = qualifiedName;
}

CppClass *CppClass::toCppClass()
{
    return this;
}

void CppClass::lookupBases(const QFuture<void> &future, Symbol *declaration,
                           const LookupContext &context)
{
    ClassOrNamespace *hierarchy = context.lookupType(declaration);
    if (!hierarchy)
        return;
    QSet<ClassOrNamespace *> visited;
    addBaseHierarchy(future, context, hierarchy, &visited);
}

void CppClass::addBaseHierarchy(const QFuture<void> &future, const LookupContext &context,
                                ClassOrNamespace *hierarchy, QSet<ClassOrNamespace *> *visited)
{
    if (future.isCanceled())
        return;
    visited->insert(hierarchy);
    const QList<ClassOrNamespace *> &baseClasses = hierarchy->usings();
    for (ClassOrNamespace *baseClass : baseClasses) {
        const QList<Symbol *> &symbols = baseClass->symbols();
        for (Symbol *symbol : symbols) {
            if (!symbol->asClass())
                continue;
            ClassOrNamespace *baseHierarchy = context.lookupType(symbol);
            if (baseHierarchy && !visited->contains(baseHierarchy)) {
                CppClass classSymbol(symbol);
                classSymbol.addBaseHierarchy(future, context, baseHierarchy, visited);
                bases.append(classSymbol);
            }
        }
    }
}

void CppClass::lookupDerived(const QFuture<void> &future, Symbol *declaration,
                             const Snapshot &snapshot)
{
    snapshot.updateDependencyTable(future);
    if (future.isCanceled())
        return;
    addDerivedHierarchy(TypeHierarchyBuilder::buildDerivedTypeHierarchy(
                        declaration, snapshot, future));
}

void CppClass::addDerivedHierarchy(const TypeHierarchy &hierarchy)
{
    const QList<TypeHierarchy> derivedHierarchies = hierarchy.hierarchy();
    for (const TypeHierarchy &derivedHierarchy : derivedHierarchies) {
        CppClass classSymbol(derivedHierarchy.symbol());
        classSymbol.addDerivedHierarchy(derivedHierarchy);
        derived.append(classSymbol);
    }
}

class CppFunction : public CppDeclarableElement
{
public:
    explicit CppFunction(Symbol *declaration)
        : CppDeclarableElement(declaration)
    {
        helpCategory = Core::HelpItem::Function;

        const FullySpecifiedType &type = declaration->type();

        // Functions marks can be found either by the main overload or signature based
        // (with no argument names and no return). Help ids have no signature at all.
        Overview overview;
        overview.showDefaultArguments = false;
        helpMark = overview.prettyType(type, name);

        overview.showFunctionSignatures = false;
        helpIdCandidates.append(overview.prettyName(declaration->name()));
    }
};

class CppEnum : public CppDeclarableElement
{
public:
    explicit CppEnum(Enum *declaration)
        : CppDeclarableElement(declaration)
    {
        helpCategory = Core::HelpItem::Enum;
        tooltip = qualifiedName;
    }
};

class CppTypedef : public CppDeclarableElement
{
public:
    explicit CppTypedef(Symbol *declaration)
        : CppDeclarableElement(declaration)
    {
        helpCategory = Core::HelpItem::Typedef;
        Overview overview;
        overview.showTemplateParameters = true;
        tooltip = overview.prettyType(declaration->type(), qualifiedName);
    }
};

class CppVariable : public CppDeclarableElement
{
public:
    explicit CppVariable(Symbol *declaration, const LookupContext &context, Scope *scope)
        : CppDeclarableElement(declaration)
    {
        const FullySpecifiedType &type = declaration->type();

        const Name *typeName = nullptr;
        if (type->asNamedType()) {
            typeName = type->asNamedType()->name();
        } else if (type->asPointerType() || type->asReferenceType()) {
            FullySpecifiedType associatedType;
            if (type->asPointerType())
                associatedType = type->asPointerType()->elementType();
            else
                associatedType = type->asReferenceType()->elementType();
            if (associatedType->asNamedType())
                typeName = associatedType->asNamedType()->name();
        }

        if (typeName) {
            if (ClassOrNamespace *clazz = context.lookupType(typeName, scope)) {
                if (!clazz->symbols().isEmpty()) {
                    Overview overview;
                    Symbol *symbol = clazz->symbols().at(0);
                    const QString &name = overview.prettyName(
                        LookupContext::fullyQualifiedName(symbol));
                    if (!name.isEmpty()) {
                        tooltip = name;
                        helpCategory = Core::HelpItem::ClassOrNamespace;
                        const QStringList &allNames = stripName(name);
                        if (!allNames.isEmpty()) {
                            helpMark = allNames.last();
                            helpIdCandidates = allNames;
                        }
                    }
                }
            }
        }
    }
};

class CppEnumerator : public CppDeclarableElement
{
public:
    explicit CppEnumerator(EnumeratorDeclaration *declaration)
        : CppDeclarableElement(declaration)
    {
        helpCategory = Core::HelpItem::Enum;

        Overview overview;

        Symbol *enumSymbol = declaration->enclosingScope();
        const QString enumName = overview.prettyName(LookupContext::fullyQualifiedName(enumSymbol));
        const QString enumeratorName = overview.prettyName(declaration->name());
        QString enumeratorValue;
        if (const StringLiteral *value = declaration->constantValue())
            enumeratorValue = QString::fromUtf8(value->chars(), value->size());

        helpMark = overview.prettyName(enumSymbol->name());

        tooltip = enumeratorName;
        if (!enumName.isEmpty())
            tooltip.prepend(enumName + QLatin1Char(' '));
        if (!enumeratorValue.isEmpty())
            tooltip.append(QLatin1String(" = ") + enumeratorValue);
    }
};

static bool isCppClass(Symbol *symbol)
{
    return symbol->asClass() || symbol->asForwardClassDeclaration()
            || (symbol->asTemplate() && symbol->asTemplate()->declaration()
                && (symbol->asTemplate()->declaration()->asClass()
                    || symbol->asTemplate()->declaration()->asForwardClassDeclaration()));
}

static Symbol *followClassDeclaration(Symbol *symbol, const Snapshot &snapshot, SymbolFinder symbolFinder,
                               LookupContext *context = nullptr)
{
    if (!symbol->asForwardClassDeclaration())
        return symbol;

    Symbol *classDeclaration = symbolFinder.findMatchingClassDeclaration(symbol, snapshot);
    if (!classDeclaration)
        return symbol;

    if (context) {
        const Document::Ptr declarationDocument = snapshot.document(classDeclaration->filePath());
        if (declarationDocument != context->thisDocument())
            (*context) = LookupContext(declarationDocument, snapshot);
    }
    return classDeclaration;
}

static Symbol *followTemplateAsClass(Symbol *symbol)
{
    if (Template *t = symbol->asTemplate(); t && t->declaration() && t->declaration()->asClass())
        return t->declaration()->asClass();
    return symbol;
}

static void createTypeHierarchy(QPromise<std::shared_ptr<CppElement>> &promise,
                                const ExecData &execData, SymbolFinder symbolFinder)
{
    if (promise.isCanceled())
        return;

    Symbol *declaration = execData.lookupItem.declaration();
    if (!declaration)
        return;

    if (!isCppClass(declaration))
        return;

    LookupContext contextToUse = execData.context;
    declaration = followClassDeclaration(declaration, execData.snapshot, symbolFinder, &contextToUse);
    declaration = followTemplateAsClass(declaration);

    if (promise.isCanceled())
        return;
    std::shared_ptr<CppClass> cppClass(new CppClass(declaration));
    const QFuture<void> future = QFuture<void>(promise.future());
    cppClass->lookupBases(future, declaration, contextToUse);
    if (promise.isCanceled())
        return;
    cppClass->lookupDerived(future, declaration, execData.snapshot);
    if (promise.isCanceled())
        return;
    promise.addResult(cppClass);
}

static std::shared_ptr<CppElement> handleLookupItemMatch(const ExecData &execData,
                                                         SymbolFinder symbolFinder)
{
    std::shared_ptr<CppElement> element;
    Symbol *declaration = execData.lookupItem.declaration();
    if (!declaration) {
        const QString &type = Overview().prettyType(execData.lookupItem.type(), QString());
        element = std::shared_ptr<CppElement>(new Unknown(type));
    } else {
        const FullySpecifiedType &type = declaration->type();
        if (declaration->asNamespace()) {
            element = std::shared_ptr<CppElement>(new CppNamespace(declaration));
        } else if (isCppClass(declaration)) {
            LookupContext contextToUse = execData.context;
            declaration = followClassDeclaration(declaration, execData.snapshot, symbolFinder, &contextToUse);
            element = std::shared_ptr<CppElement>(new CppClass(declaration));
        } else if (Enum *enumDecl = declaration->asEnum()) {
            element = std::shared_ptr<CppElement>(new CppEnum(enumDecl));
        } else if (auto enumerator = dynamic_cast<EnumeratorDeclaration *>(declaration)) {
            element = std::shared_ptr<CppElement>(new CppEnumerator(enumerator));
        } else if (declaration->isTypedef()) {
            element = std::shared_ptr<CppElement>(new CppTypedef(declaration));
        } else if (declaration->asFunction()
                   || (type.isValid() && type->asFunctionType())
                   || declaration->asTemplate()) {
            element = std::shared_ptr<CppElement>(new CppFunction(declaration));
        } else if (declaration->asDeclaration() && type.isValid()) {
            element = std::shared_ptr<CppElement>(
                new CppVariable(declaration, execData.context, execData.lookupItem.scope()));
        } else {
            element = std::shared_ptr<CppElement>(new CppDeclarableElement(declaration));
        }
    }
    return element;
}

//  special case for bug QTCREATORBUG-4780
static bool shouldOmitElement(const LookupItem &lookupItem, const Scope *scope)
{
    return !lookupItem.declaration() && scope && scope->asFunction()
            && lookupItem.type().match(scope->asFunction()->returnType());
}

static QFuture<std::shared_ptr<CppElement>> createFinishedFuture()
{
    QFutureInterface<std::shared_ptr<CppElement>> futureInterface;
    futureInterface.reportStarted();
    futureInterface.reportFinished();
    return futureInterface.future();
}

static LookupItem findLookupItem(const CPlusPlus::Snapshot &snapshot, const SourceData &sourceData,
                                 LookupContext *lookupContext, bool followTypedef)
{
    TypeOfExpression typeOfExpression;
    typeOfExpression.init(sourceData.doc, snapshot);
    // make possible to instantiate templates
    typeOfExpression.setExpandTemplates(true);
    const QList<LookupItem> &lookupItems = typeOfExpression(sourceData.expression.toUtf8(),
                                                            sourceData.scope);
    *lookupContext = typeOfExpression.context();
    if (lookupItems.isEmpty())
        return LookupItem();

    auto isInteresting = [followTypedef](Symbol *symbol) {
        return symbol && (!followTypedef || (symbol->asClass() || symbol->asTemplate()
               || symbol->asForwardClassDeclaration() || symbol->isTypedef()));
    };

    for (const LookupItem &item : lookupItems) {
        if (shouldOmitElement(item, sourceData.scope))
            continue;
        Symbol *symbol = item.declaration();
        if (!isInteresting(symbol))
            continue;
        if (followTypedef && symbol->isTypedef()) {
            CPlusPlus::NamedType *namedType = symbol->type()->asNamedType();
            if (!namedType) {
                // Anonymous aggregate such as: typedef struct {} Empty;
                continue;
            }
            return TypeHierarchyBuilder::followTypedef(*lookupContext,
                         namedType->name(), symbol->enclosingScope());
        }
        return item;
    }
    return LookupItem();
}

static QFuture<std::shared_ptr<CppElement>> exec(SourceFunction &&sourceFunction,
                                                 ExecFunction &&execFunction,
                                                 bool followTypedef = true)
{
    const Snapshot &snapshot = CppModelManager::snapshot();

    const auto inputData = std::invoke(std::forward<SourceFunction>(sourceFunction), snapshot);
    if (!inputData)
        return createFinishedFuture();

    LookupContext lookupContext;
    const LookupItem &lookupItem = findLookupItem(snapshot, *inputData, &lookupContext, followTypedef);
    if (!lookupItem.declaration())
        return createFinishedFuture();

    return std::invoke(std::forward<ExecFunction>(execFunction), ExecData{snapshot, lookupItem, lookupContext});
}

static QFuture<std::shared_ptr<CppElement>> asyncExec(const ExecData &execData)
{
    return Utils::asyncRun(&createTypeHierarchy, execData, *CppModelManager::symbolFinder());
}

class FromExpressionFunctor
{
public:
    FromExpressionFunctor(const QString &expression, const FilePath &filePath)
        : m_expression(expression)
        , m_filePath(filePath)
    {}

    std::optional<SourceData> operator()(const CPlusPlus::Snapshot &snapshot)
    {
        Document::Ptr doc = snapshot.document(m_filePath);
        if (doc.isNull())
            return {};

        return SourceData{doc, doc->globalNamespace(), m_expression};
    }
private:
    const QString m_expression;
    const FilePath m_filePath;
};

QFuture<std::shared_ptr<CppElement>> CppElementEvaluator::asyncExecute(const QString &expression,
                                                                       const FilePath &filePath)
{
    return exec(FromExpressionFunctor(expression, filePath), asyncExec);
}

class FromGuiFunctor
{
public:
    FromGuiFunctor(TextEditor::TextEditorWidget *editor)
        : m_editor(editor)
        , m_tc(editor->textCursor())
    {}

    std::optional<SourceData> operator()(const CPlusPlus::Snapshot &snapshot)
    {
        Document::Ptr doc;
        doc = snapshot.document(m_editor->textDocument()->filePath());
        if (!doc)
            return {};

        int line = 0;
        int column = 0;
        const int pos = m_tc.position();
        m_editor->convertPosition(pos, &line, &column);

        checkDiagnosticMessage(pos);

        if (matchIncludeFile(doc, line) || matchMacroInUse(doc, pos))
            return {};

        moveCursorToEndOfIdentifier(&m_tc);
        ExpressionUnderCursor expressionUnderCursor(doc->languageFeatures());
        return SourceData{doc, doc->scopeAt(line, column), expressionUnderCursor(m_tc)};
    }
    QFuture<std::shared_ptr<CppElement>> syncExec(const ExecData &execData);

private:
    void checkDiagnosticMessage(int pos);
    bool matchIncludeFile(const CPlusPlus::Document::Ptr &document, int line);
    bool matchMacroInUse(const CPlusPlus::Document::Ptr &document, int pos);

public:
    void clear();

    TextEditor::TextEditorWidget *m_editor;
    QTextCursor m_tc;
    std::shared_ptr<CppElement> m_element;
    QString m_diagnosis;
};

QFuture<std::shared_ptr<CppElement>> FromGuiFunctor::syncExec(const ExecData &execData)
{
    QFutureInterface<std::shared_ptr<CppElement>> futureInterface;
    futureInterface.reportStarted();
    m_element = handleLookupItemMatch(execData, *CppModelManager::symbolFinder());
    futureInterface.reportResult(m_element);
    futureInterface.reportFinished();
    return futureInterface.future();
}

void FromGuiFunctor::checkDiagnosticMessage(int pos)
{
    const QList<QTextEdit::ExtraSelection> &selections = m_editor->extraSelections(
        TextEditor::TextEditorWidget::CodeWarningsSelection);
    for (const QTextEdit::ExtraSelection &sel : selections) {
        if (pos >= sel.cursor.selectionStart() && pos <= sel.cursor.selectionEnd()) {
            m_diagnosis = sel.format.toolTip();
            break;
        }
    }
}

bool FromGuiFunctor::matchIncludeFile(const Document::Ptr &document, int line)
{
    const QList<Document::Include> &includes = document->resolvedIncludes();
    for (const Document::Include &includeFile : includes) {
        if (includeFile.line() == line) {
            m_element = std::shared_ptr<CppElement>(new CppInclude(includeFile));
            return true;
        }
    }
    return false;
}

bool FromGuiFunctor::matchMacroInUse(const Document::Ptr &document, int pos)
{
    for (const Document::MacroUse &use : document->macroUses()) {
        if (use.containsUtf16charOffset(pos)) {
            const int begin = use.utf16charsBegin();
            if (pos < begin + use.macro().nameToQString().size()) {
                m_element = std::shared_ptr<CppElement>(new CppMacro(use.macro()));
                return true;
            }
        }
    }
    return false;
}

void FromGuiFunctor::clear()
{
    m_element.reset();
    m_diagnosis.clear();
}

class CppElementEvaluatorPrivate
{
public:
    CppElementEvaluatorPrivate(TextEditor::TextEditorWidget *editor) : m_functor(editor) {}
    FromGuiFunctor m_functor;
};

CppElementEvaluator::CppElementEvaluator(TextEditor::TextEditorWidget *editor)
    : d(new CppElementEvaluatorPrivate(editor))
{}

CppElementEvaluator::~CppElementEvaluator()
{
    delete d;
}

void CppElementEvaluator::setTextCursor(const QTextCursor &tc)
{
    d->m_functor.m_tc = tc;
}

QFuture<std::shared_ptr<CppElement>> CppElementEvaluator::asyncExecute(
        TextEditor::TextEditorWidget *editor)
{
    return exec(FromGuiFunctor(editor), asyncExec);
}

void CppElementEvaluator::execute()
{
    d->m_functor.clear();
    const auto execFunction = [this](const ExecData &execData) {
        return d->m_functor.syncExec(execData);
    };
    exec(std::ref(d->m_functor), execFunction, false);
}

const std::shared_ptr<CppElement> &CppElementEvaluator::cppElement() const
{
    return d->m_functor.m_element;
}

bool CppElementEvaluator::hasDiagnosis() const
{
    return !d->m_functor.m_diagnosis.isEmpty();
}

const QString &CppElementEvaluator::diagnosis() const
{
    return d->m_functor.m_diagnosis;
}

Utils::Link CppElementEvaluator::linkFromExpression(const QString &expression, const FilePath &filePath)
{
    const Snapshot &snapshot = CppModelManager::snapshot();
    Document::Ptr doc = snapshot.document(filePath);
    if (doc.isNull())
        return Utils::Link();
    Scope *scope = doc->globalNamespace();

    TypeOfExpression typeOfExpression;
    typeOfExpression.init(doc, snapshot);
    typeOfExpression.setExpandTemplates(true);
    const QList<LookupItem> &lookupItems = typeOfExpression(expression.toUtf8(), scope);
    if (lookupItems.isEmpty())
        return Utils::Link();

    for (const LookupItem &item : lookupItems) {
        Symbol *symbol = item.declaration();
        if (!symbol)
            continue;
        if (!symbol->asClass() && !symbol->asTemplate())
            continue;
        return symbol->toLink();
    }
    return Utils::Link();
}

} // namespace CppEditor::Internal
