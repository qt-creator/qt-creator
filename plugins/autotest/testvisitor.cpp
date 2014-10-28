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

#include <QList>

namespace Autotest {
namespace Internal {

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

        if (auto func = type->asFunctionType()) {
            if (func->isSlot() && member->isPrivate()) {
                const QString name = o.prettyName(func->name());
                if (!ignoredFunctions.contains(name) && !name.endsWith(QLatin1String("_data"))) {
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

    if (auto expressionAST = ast->base_expression) {
        if (auto idExpressionAST = expressionAST->asIdExpression()) {
            if (auto qualifiedNameAST = idExpressionAST->name->asQualifiedName()) {
                const CPlusPlus::Overview o;
                const QString prettyName = o.prettyName(qualifiedNameAST->name);
                if (prettyName == QLatin1String("QTest::qExec")) {
                    if (auto expressionListAST = ast->expression_list) {
                        // first argument is the one we need
                        if (auto argumentExpressionAST = expressionListAST->value) {
                            CPlusPlus::TypeOfExpression toe;
                            CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
                            toe.init(m_currentDoc, cppMM->snapshot());
                            QList<CPlusPlus::LookupItem> toeItems
                                    = toe(argumentExpressionAST, m_currentDoc, m_currentScope);

                            if (toeItems.size()) {
                                if (auto pointerType = toeItems.first().type()->asPointerType())
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

} // namespace Internal
} // namespace Autotest
