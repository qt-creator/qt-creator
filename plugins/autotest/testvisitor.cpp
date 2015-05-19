/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
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

// names of special functions (applies for QTest as well as Quick Tests)
static QList<QString> specialFunctions = QList<QString>() << QLatin1String("initTestCase")
                                                          << QLatin1String("cleanupTestCase")
                                                          << QLatin1String("init")
                                                          << QLatin1String("cleanup");

/************************** Cpp Test Symbol Visitor ***************************/

TestVisitor::TestVisitor(const QString &fullQualifiedClassName)
    : m_className(fullQualifiedClassName)
{
}

TestVisitor::~TestVisitor()
{
}

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
                // TODO use definition of function instead of declaration!
                TestCodeLocationAndType locationAndType;
                locationAndType.m_fileName = QLatin1String(member->fileName());
                locationAndType.m_line = member->line();
                locationAndType.m_column = member->column() - 1;
                if (specialFunctions.contains(name))
                    locationAndType.m_type = TestTreeItem::TEST_SPECIALFUNCTION;
                else if (name.endsWith(QLatin1String("_data")))
                    locationAndType.m_type = TestTreeItem::TEST_DATAFUNCTION;
                else
                    locationAndType.m_type = TestTreeItem::TEST_FUNCTION;
                m_privSlots.insert(name, locationAndType);
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
        return true; // find nested TestCase items as well

    m_currentTestCaseName.clear();
    const auto sourceLocation = ast->firstSourceLocation();
    m_testCaseLocation.m_fileName = m_currentDoc->fileName();
    m_testCaseLocation.m_line = sourceLocation.startLine;
    m_testCaseLocation.m_column = sourceLocation.startColumn - 1;
    m_testCaseLocation.m_type = TestTreeItem::TEST_CLASS;
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
    if (name.startsWith(QLatin1String("test_"))
            || name.startsWith(QLatin1String("benchmark_"))
            || name.endsWith(QLatin1String("_data"))
            || specialFunctions.contains(name.toString())) {
        const auto sourceLocation = ast->firstSourceLocation();
        TestCodeLocationAndType locationAndType;
        locationAndType.m_fileName = m_currentDoc->fileName();
        locationAndType.m_line = sourceLocation.startLine;
        locationAndType.m_column = sourceLocation.startColumn - 1;
        if (specialFunctions.contains(name.toString()))
            locationAndType.m_type = TestTreeItem::TEST_SPECIALFUNCTION;
        else if (name.endsWith(QLatin1String("_data")))
            locationAndType.m_type = TestTreeItem::TEST_DATAFUNCTION;
        else
            locationAndType.m_type = TestTreeItem::TEST_FUNCTION;

        m_testFunctions.insert(name.toString(), locationAndType);
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
