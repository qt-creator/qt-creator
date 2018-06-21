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

#include "quicktestvisitors.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljsutils.h>
#include <utils/algorithm.h>

namespace Autotest {
namespace Internal {

static QStringList specialFunctions({"initTestCase", "cleanupTestCase", "init", "cleanup"});

TestQmlVisitor::TestQmlVisitor(QmlJS::Document::Ptr doc, const QmlJS::Snapshot &snapshot)
    : m_currentDoc(doc)
    , m_snapshot(snapshot)
{
}

static bool documentImportsQtTest(const QmlJS::Document *doc)
{
    if (const QmlJS::Bind *bind = doc->bind()) {
        return Utils::anyOf(bind->imports(), [] (const QmlJS::ImportInfo &info) {
            return info.isValid() && info.name() == "QtTest";
        });
    }
    return false;
}

static bool isDerivedFromTestCase(QmlJS::AST::UiQualifiedId *id, const QmlJS::Document::Ptr &doc,
                                  const QmlJS::Snapshot &snapshot)
{
    if (!id)
        return false;
    QmlJS::Link link(snapshot, QmlJS::ViewerContext(), QmlJS::LibraryInfo());
    const QmlJS::ContextPtr context = link();

    const QmlJS::ObjectValue *value = context->lookupType(doc.data(), id);
    if (!value)
        return false;

    QmlJS::PrototypeIterator protoIterator(value, context);
    const QList<const QmlJS::ObjectValue *> prototypes = protoIterator.all();
    for (const QmlJS::ObjectValue *val : prototypes) {
        if (auto prototype = val->prototype()) {
            if (auto qmlPrototypeRef = prototype->asQmlPrototypeReference()) {
                if (auto qmlTypeName = qmlPrototypeRef->qmlTypeName()) {
                    if (qmlTypeName->name == "TestCase") {
                        if (auto astObjVal = val->asAstObjectValue())
                            return documentImportsQtTest(astObjVal->document());
                    }
                }
            }
        }
    }
    return false;
}

bool TestQmlVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    const QStringRef name = ast->qualifiedTypeNameId->name;
    if (name != "TestCase") {
        if (!isDerivedFromTestCase(ast->qualifiedTypeNameId, m_currentDoc, m_snapshot))
            return true;
    } else if (!documentImportsQtTest(m_currentDoc.data())) {
        return true; // find nested TestCase items as well
    }

    m_typeIsTestCase = true;
    m_currentTestCaseName.clear();
    const auto sourceLocation = ast->firstSourceLocation();
    m_testCaseLocation.m_name = m_currentDoc->fileName();
    m_testCaseLocation.m_line = sourceLocation.startLine;
    m_testCaseLocation.m_column = sourceLocation.startColumn - 1;
    m_testCaseLocation.m_type = TestTreeItem::TestCase;
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
    return name == "name";
}

bool TestQmlVisitor::visit(QmlJS::AST::FunctionDeclaration *ast)
{
    const QStringRef name = ast->name;
    if (name.startsWith("test_")
            || name.startsWith("benchmark_")
            || name.endsWith("_data")
            || specialFunctions.contains(name.toString())) {
        const auto sourceLocation = ast->firstSourceLocation();
        TestCodeLocationAndType locationAndType;
        locationAndType.m_name = m_currentDoc->fileName();
        locationAndType.m_line = sourceLocation.startLine;
        locationAndType.m_column = sourceLocation.startColumn - 1;
        if (specialFunctions.contains(name.toString()))
            locationAndType.m_type = TestTreeItem::TestSpecialFunction;
        else if (name.endsWith("_data"))
            locationAndType.m_type = TestTreeItem::TestDataFunction;
        else
            locationAndType.m_type = TestTreeItem::TestFunctionOrSet;

        m_testFunctions.insert(name.toString(), locationAndType);
    }
    return false;
}

bool TestQmlVisitor::visit(QmlJS::AST::StringLiteral *ast)
{
    if (m_typeIsTestCase)
        m_currentTestCaseName = ast->value.toString();
    return false;
}

} // namespace Internal
} // namespace Autotest
