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

#include "cppelementevaluator.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/symbolfinder.h>
#include <cpptools/typehierarchybuilder.h>

#include <texteditor/textdocument.h>

#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Icons.h>
#include <cplusplus/TypeOfExpression.h>

#include <utils/runextensions.h>

#include <QDir>
#include <QSet>
#include <QQueue>

using namespace CPlusPlus;

namespace CppTools {

static void handleLookupItemMatch(QFutureInterface<QSharedPointer<CppElement>> &futureInterface,
                                  const Snapshot &snapshot,
                                  const LookupItem &lookupItem,
                                  const LookupContext &context,
                                  SymbolFinder symbolFinder,
                                  bool lookupBaseClasses,
                                  bool lookupDerivedClasses);

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
        : path(QDir::toNativeSeparators(includeFile.resolvedFileName()))
        , fileName(Utils::FilePath::fromString(includeFile.resolvedFileName()).fileName())
    {
        helpCategory = Core::HelpItem::Brief;
        helpIdCandidates = QStringList(fileName);
        helpMark = fileName;
        link = Utils::Link(path);
        tooltip = path;
    }

public:
    QString path;
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
        link = Utils::Link(macro.fileName(), macro.line());
        tooltip = macro.toStringWithLineBreaks();
    }
};

// CppDeclarableElement
CppDeclarableElement::CppDeclarableElement(Symbol *declaration)
    : CppElement()
    , declaration(declaration)
    , icon(Icons::iconForSymbol(declaration))
{
    Overview overview;
    overview.showArgumentNames = true;
    overview.showReturnTypes = true;
    name = overview.prettyName(declaration->name());
    if (declaration->enclosingScope()->isClass() ||
        declaration->enclosingScope()->isNamespace() ||
        declaration->enclosingScope()->isEnum()) {
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

bool CppClass::operator==(const CppClass &other)
{
    return this->declaration == other.declaration;
}

CppClass *CppClass::toCppClass()
{
    return this;
}

void CppClass::lookupBases(QFutureInterfaceBase &futureInterface,
                           Symbol *declaration, const LookupContext &context)
{
    using Data = QPair<ClassOrNamespace*, CppClass*>;

    if (ClassOrNamespace *clazz = context.lookupType(declaration)) {
        QSet<ClassOrNamespace *> visited;

        QQueue<Data> q;
        q.enqueue(qMakePair(clazz, this));
        while (!q.isEmpty()) {
            if (futureInterface.isCanceled())
                return;
            Data current = q.dequeue();
            clazz = current.first;
            visited.insert(clazz);
            const QList<ClassOrNamespace *> &bases = clazz->usings();
            foreach (ClassOrNamespace *baseClass, bases) {
                const QList<Symbol *> &symbols = baseClass->symbols();
                foreach (Symbol *symbol, symbols) {
                    if (symbol->isClass() && (
                        clazz = context.lookupType(symbol)) &&
                        !visited.contains(clazz)) {
                        CppClass baseCppClass(symbol);
                        CppClass *cppClass = current.second;
                        cppClass->bases.append(baseCppClass);
                        q.enqueue(qMakePair(clazz, &cppClass->bases.last()));
                    }
                }
            }
        }
    }
}

void CppClass::lookupDerived(QFutureInterfaceBase &futureInterface,
                             Symbol *declaration, const Snapshot &snapshot)
{
    using Data = QPair<CppClass*, TypeHierarchy>;

    snapshot.updateDependencyTable(futureInterface);
    if (futureInterface.isCanceled())
        return;
    const TypeHierarchy &completeHierarchy
            = TypeHierarchyBuilder::buildDerivedTypeHierarchy(futureInterface, declaration, snapshot);

    QQueue<Data> q;
    q.enqueue(qMakePair(this, completeHierarchy));
    while (!q.isEmpty()) {
        if (futureInterface.isCanceled())
            return;
        const Data &current = q.dequeue();
        CppClass *clazz = current.first;
        const TypeHierarchy &classHierarchy = current.second;
        foreach (const TypeHierarchy &derivedHierarchy, classHierarchy.hierarchy()) {
            clazz->derived.append(CppClass(derivedHierarchy.symbol()));
            q.enqueue(qMakePair(&clazz->derived.last(), derivedHierarchy));
        }
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
        tooltip = Overview().prettyType(declaration->type(), qualifiedName);
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
        if (type->isNamedType()) {
            typeName = type->asNamedType()->name();
        } else if (type->isPointerType() || type->isReferenceType()) {
            FullySpecifiedType associatedType;
            if (type->isPointerType())
                associatedType = type->asPointerType()->elementType();
            else
                associatedType = type->asReferenceType()->elementType();
            if (associatedType->isNamedType())
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

CppElementEvaluator::CppElementEvaluator(TextEditor::TextEditorWidget *editor) :
    m_editor(editor),
    m_modelManager(CppModelManager::instance()),
    m_tc(editor ? editor->textCursor() : QTextCursor()),
    m_lookupBaseClasses(false),
    m_lookupDerivedClasses(false)
{}

void CppElementEvaluator::setTextCursor(const QTextCursor &tc)
{ m_tc = tc; }

void CppElementEvaluator::setLookupBaseClasses(const bool lookup)
{ m_lookupBaseClasses = lookup; }

void CppElementEvaluator::setLookupDerivedClasses(const bool lookup)
{ m_lookupDerivedClasses = lookup; }

void CppElementEvaluator::setExpression(const QString &expression, const QString &fileName)
{
    m_expression = expression;
    m_fileName = fileName;
}

//  special case for bug QTCREATORBUG-4780
static bool shouldOmitElement(const LookupItem &lookupItem, const Scope *scope)
{
    return !lookupItem.declaration() && scope && scope->isFunction()
            && lookupItem.type().match(scope->asFunction()->returnType());
}

void CppElementEvaluator::execute()
{
    execute(&CppElementEvaluator::sourceDataFromGui, &CppElementEvaluator::syncExec);
}

QFuture<QSharedPointer<CppElement>> CppElementEvaluator::asyncExecute()
{
    return execute(&CppElementEvaluator::sourceDataFromGui, &CppElementEvaluator::asyncExec);
}

QFuture<QSharedPointer<CppElement>> CppElementEvaluator::asyncExpressionExecute()
{
    return execute(&CppElementEvaluator::sourceDataFromExpression, &CppElementEvaluator::asyncExec);
}

static QFuture<QSharedPointer<CppElement>> createFinishedFuture()
{
    QFutureInterface<QSharedPointer<CppElement>> futureInterface;
    futureInterface.reportStarted();
    futureInterface.reportFinished();
    return futureInterface.future();
}

bool CppElementEvaluator::sourceDataFromGui(const CPlusPlus::Snapshot &snapshot,
                                            Document::Ptr &doc, Scope **scope, QString &expression)
{
    doc = snapshot.document(m_editor->textDocument()->filePath());
    if (!doc)
        return false;

    int line = 0;
    int column = 0;
    const int pos = m_tc.position();
    m_editor->convertPosition(pos, &line, &column);

    checkDiagnosticMessage(pos);

    if (matchIncludeFile(doc, line) || matchMacroInUse(doc, pos))
        return false;

    CppTools::moveCursorToEndOfIdentifier(&m_tc);
    ExpressionUnderCursor expressionUnderCursor(doc->languageFeatures());
    expression = expressionUnderCursor(m_tc);

    // Fetch the expression's code
    *scope = doc->scopeAt(line, column - 1);
    return true;
}

bool CppElementEvaluator::sourceDataFromExpression(const CPlusPlus::Snapshot &snapshot,
                          Document::Ptr &doc, Scope **scope, QString &expression)
{
    doc = snapshot.document(m_fileName);
    expression = m_expression;

    // Fetch the expression's code
    *scope = doc->globalNamespace();
    return true;
}

// @todo: Consider refactoring code from CppEditor::findLinkAt into here.
QFuture<QSharedPointer<CppElement>> CppElementEvaluator::execute(SourceFunction sourceFunction,
                                                                 ExecFunction execFuntion)
{
    clear();

    if (!m_modelManager)
        return createFinishedFuture();

    const Snapshot &snapshot = m_modelManager->snapshot();

    Document::Ptr doc;
    QString expression;
    Scope *scope = nullptr;
    if (!std::invoke(sourceFunction, this, snapshot, doc, &scope, expression))
        return createFinishedFuture();

    TypeOfExpression typeOfExpression;
    typeOfExpression.init(doc, snapshot);
    // make possible to instantiate templates
    typeOfExpression.setExpandTemplates(true);
    const QList<LookupItem> &lookupItems = typeOfExpression(expression.toUtf8(), scope);
    if (lookupItems.isEmpty())
        return createFinishedFuture();

    const LookupItem &lookupItem = lookupItems.first(); // ### TODO: select best candidate.
    if (shouldOmitElement(lookupItem, scope))
        return createFinishedFuture();
    return std::invoke(execFuntion, this, snapshot, lookupItem, typeOfExpression.context());
}

QFuture<QSharedPointer<CppElement>> CppElementEvaluator::syncExec(
        const CPlusPlus::Snapshot &snapshot, const CPlusPlus::LookupItem &lookupItem,
        const CPlusPlus::LookupContext &lookupContext)
{
    QFutureInterface<QSharedPointer<CppElement>> futureInterface;
    futureInterface.reportStarted();
    handleLookupItemMatch(futureInterface, snapshot, lookupItem, lookupContext,
                          *m_modelManager->symbolFinder(),
                          m_lookupBaseClasses, m_lookupDerivedClasses);
    futureInterface.reportFinished();
    QFuture<QSharedPointer<CppElement>> future = futureInterface.future();
    m_element = future.result();
    return future;
}

QFuture<QSharedPointer<CppElement>> CppElementEvaluator::asyncExec(
        const CPlusPlus::Snapshot &snapshot, const CPlusPlus::LookupItem &lookupItem,
        const CPlusPlus::LookupContext &lookupContext)
{
    return Utils::runAsync(&handleLookupItemMatch, snapshot, lookupItem, lookupContext,
                           *m_modelManager->symbolFinder(),
                           m_lookupBaseClasses, m_lookupDerivedClasses);
}

void CppElementEvaluator::checkDiagnosticMessage(int pos)
{
    foreach (const QTextEdit::ExtraSelection &sel,
             m_editor->extraSelections(TextEditor::TextEditorWidget::CodeWarningsSelection)) {
        if (pos >= sel.cursor.selectionStart() && pos <= sel.cursor.selectionEnd()) {
            m_diagnosis = sel.format.toolTip();
            break;
        }
    }
}

bool CppElementEvaluator::matchIncludeFile(const Document::Ptr &document, int line)
{
    foreach (const Document::Include &includeFile, document->resolvedIncludes()) {
        if (includeFile.line() == line) {
            m_element = QSharedPointer<CppElement>(new CppInclude(includeFile));
            return true;
        }
    }
    return false;
}

bool CppElementEvaluator::matchMacroInUse(const Document::Ptr &document, int pos)
{
    foreach (const Document::MacroUse &use, document->macroUses()) {
        if (use.containsUtf16charOffset(pos)) {
            const int begin = use.utf16charsBegin();
            if (pos < begin + use.macro().nameToQString().size()) {
                m_element = QSharedPointer<CppElement>(new CppMacro(use.macro()));
                return true;
            }
        }
    }
    return false;
}

static void handleLookupItemMatch(QFutureInterface<QSharedPointer<CppElement>> &futureInterface,
                                  const Snapshot &snapshot,
                                  const LookupItem &lookupItem,
                                  const LookupContext &context,
                                  SymbolFinder symbolFinder,
                                  bool lookupBaseClasses,
                                  bool lookupDerivedClasses)
{
    if (futureInterface.isCanceled())
        return;
    QSharedPointer<CppElement> element;
    Symbol *declaration = lookupItem.declaration();
    if (!declaration) {
        const QString &type = Overview().prettyType(lookupItem.type(), QString());
        element = QSharedPointer<CppElement>(new Unknown(type));
    } else {
        const FullySpecifiedType &type = declaration->type();
        if (declaration->isNamespace()) {
            element = QSharedPointer<CppElement>(new CppNamespace(declaration));
        } else if (declaration->isClass()
                   || declaration->isForwardClassDeclaration()
                   || (declaration->isTemplate() && declaration->asTemplate()->declaration()
                       && (declaration->asTemplate()->declaration()->isClass()
                           || declaration->asTemplate()->declaration()->isForwardClassDeclaration()))) {
            LookupContext contextToUse = context;
            if (declaration->isForwardClassDeclaration()) {
                Symbol *classDeclaration = symbolFinder.findMatchingClassDeclaration(declaration,
                                                                                      snapshot);
                if (classDeclaration) {
                    declaration = classDeclaration;
                    const QString fileName = QString::fromUtf8(declaration->fileName(),
                                                               declaration->fileNameLength());
                    const Document::Ptr declarationDocument = snapshot.document(fileName);
                    if (declarationDocument != context.thisDocument())
                        contextToUse = LookupContext(declarationDocument, snapshot);
                }
            }

            if (futureInterface.isCanceled())
                return;
            QSharedPointer<CppClass> cppClass(new CppClass(declaration));
            if (lookupBaseClasses)
                cppClass->lookupBases(futureInterface, declaration, contextToUse);
            if (futureInterface.isCanceled())
                return;
            if (lookupDerivedClasses)
                cppClass->lookupDerived(futureInterface, declaration, snapshot);
            if (futureInterface.isCanceled())
                return;
            element = cppClass;
        } else if (Enum *enumDecl = declaration->asEnum()) {
            element = QSharedPointer<CppElement>(new CppEnum(enumDecl));
        } else if (auto enumerator = dynamic_cast<EnumeratorDeclaration *>(declaration)) {
            element = QSharedPointer<CppElement>(new CppEnumerator(enumerator));
        } else if (declaration->isTypedef()) {
            element = QSharedPointer<CppElement>(new CppTypedef(declaration));
        } else if (declaration->isFunction()
                   || (type.isValid() && type->isFunctionType())
                   || declaration->isTemplate()) {
            element = QSharedPointer<CppElement>(new CppFunction(declaration));
        } else if (declaration->isDeclaration() && type.isValid()) {
            element = QSharedPointer<CppElement>(
                new CppVariable(declaration, context, lookupItem.scope()));
        } else {
            element = QSharedPointer<CppElement>(new CppDeclarableElement(declaration));
        }
    }
    futureInterface.reportResult(element);
}

bool CppElementEvaluator::identifiedCppElement() const
{
    return !m_element.isNull();
}

const QSharedPointer<CppElement> &CppElementEvaluator::cppElement() const
{
    return m_element;
}

bool CppElementEvaluator::hasDiagnosis() const
{
    return !m_diagnosis.isEmpty();
}

const QString &CppElementEvaluator::diagnosis() const
{
    return m_diagnosis;
}

void CppElementEvaluator::clear()
{
    m_element.clear();
    m_diagnosis.clear();
}

Utils::Link CppElementEvaluator::linkFromExpression(const QString &expression, const QString &fileName)
{
    const Snapshot &snapshot = CppModelManager::instance()->snapshot();
    Document::Ptr doc = snapshot.document(fileName);
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
        if (!symbol->isClass() && !symbol->isTemplate())
            continue;
        return symbol->toLink();
    }
    return Utils::Link();
}

} // namespace CppTools
