/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#include "testvisitor.h"

#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/TypeOfExpression.h>

#include <cpptools/cppmodelmanager.h>

#include <qmljs/parser/qmljsast_p.h>

#include <QList>

namespace Autotest {
namespace Internal {

/************************** Cpp Test Symbol Visitor ***************************/

TestVisitor::TestVisitor(const QString &fullQualifiedClassName)
    : m_className(fullQualifiedClassName)
{
}

TestVisitor::~TestVisitor()
{
}

static QList<QString> ignoredFunctions = QList<QString>() << QLatin1String("initTestCase")
                                                          << QLatin1String("cleanupTestCase")
                                                          << QLatin1String("init")
                                                          << QLatin1String("cleanup");

bool TestVisitor::visit(CPlusPlus::Class *symbol)
{
    const CPlusPlus::Overview o;
    CPlusPlus::LookupContext lc;

    unsigned count = symbol->memberCount();
    for (unsigned i = 0; i < count; ++i) {
        CPlusPlus::Symbol *member = symbol->memberAt(i);
        CPlusPlus::Type *type = member->type().type();

        const QString className = o.prettyName(lc.fullyQualifiedName(member->enclosingClass()));
        if (className != m_className)
            continue;

        if (const auto func = type->asFunctionType()) {
            if (func->isSlot() && member->isPrivate()) {
                const QString name = o.prettyName(func->name());
                if (!ignoredFunctions.contains(name) && !name.endsWith(QLatin1String("_data"))) {
                    // TODO use definition of function instead of declaration!
                    TestCodeLocation location;
                    location.m_fileName = QLatin1String(member->fileName());
                    location.m_line = member->line();
                    location.m_column = member->column() - 1;
                    m_privSlots.insert(name, location);
                }
            }
        }
    }
    return true;
}

/**************************** Cpp Test AST Visitor ****************************/

TestAstVisitor::TestAstVisitor(CPlusPlus::Document::Ptr doc)
    : ASTVisitor(doc->translationUnit()),
      m_currentDoc(doc)
{
}

TestAstVisitor::~TestAstVisitor()
{
}

bool TestAstVisitor::visit(CPlusPlus::CallAST *ast)
{
    if (!m_currentScope || m_currentDoc.isNull())
        return false;

    if (const auto expressionAST = ast->base_expression) {
        if (const auto idExpressionAST = expressionAST->asIdExpression()) {
            if (const auto qualifiedNameAST = idExpressionAST->name->asQualifiedName()) {
                const CPlusPlus::Overview o;
                const QString prettyName = o.prettyName(qualifiedNameAST->name);
                if (prettyName == QLatin1String("QTest::qExec")) {
                    if (const auto expressionListAST = ast->expression_list) {
                        // first argument is the one we need
                        if (const auto argumentExpressionAST = expressionListAST->value) {
                            CPlusPlus::TypeOfExpression toe;
                            CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
                            toe.init(m_currentDoc, cppMM->snapshot());
                            QList<CPlusPlus::LookupItem> toeItems
                                    = toe(argumentExpressionAST, m_currentDoc, m_currentScope);

                            if (toeItems.size()) {
                                if (const auto pointerType = toeItems.first().type()->asPointerType())
                                    m_className = o.prettyType(pointerType->elementType());
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

bool TestAstVisitor::visit(CPlusPlus::CompoundStatementAST *ast)
{
    m_currentScope = ast->symbol->asScope();
    return true;
}

/*************************** Quick Test AST Visitor ***************************/

TestQmlVisitor::TestQmlVisitor(QmlJS::Document::Ptr doc)
    : m_currentDoc(doc)
{
}

TestQmlVisitor::~TestQmlVisitor()
{
}

bool TestQmlVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    const QStringRef name = ast->qualifiedTypeNameId->name;
    if (name != QLatin1String("TestCase"))
        return false;

    m_currentTestCaseName.clear();
    const auto sourceLocation = ast->firstSourceLocation();
    m_testCaseLocation.m_fileName = m_currentDoc->fileName();
    m_testCaseLocation.m_line = sourceLocation.startLine;
    m_testCaseLocation.m_column = sourceLocation.startColumn - 1;
    return true;
}

bool TestQmlVisitor::visit(QmlJS::AST::ExpressionStatement *ast)
{
    const QmlJS::AST::ExpressionNode *expr = ast->expression;
    return expr->kind == QmlJS::AST::Node::Kind_StringLiteral;
}

bool TestQmlVisitor::visit(QmlJS::AST::UiScriptBinding *ast)
{
    const QStringRef name = ast->qualifiedId->name;
    return name == QLatin1String("name");
}

bool TestQmlVisitor::visit(QmlJS::AST::FunctionDeclaration *ast)
{
    const QStringRef name = ast->name;
    if (name.startsWith(QLatin1String("test_"))) {
        const auto sourceLocation = ast->firstSourceLocation();
        TestCodeLocation location;
        location.m_fileName = m_currentDoc->fileName();
        location.m_line = sourceLocation.startLine;
        location.m_column = sourceLocation.startColumn - 1;

        m_testFunctions.insert(name.toString(), location);
    }
    return false;
}

bool TestQmlVisitor::visit(QmlJS::AST::StringLiteral *ast)
{
    m_currentTestCaseName = ast->value.toString();
    return false;
}

} // namespace Internal
} // namespace Autotest
