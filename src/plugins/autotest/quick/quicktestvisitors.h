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

#pragma once

#include "quicktesttreeitem.h"

#include <cplusplus/ASTVisitor.h>
#include <cplusplus/CppDocument.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>

#include <QStack>

namespace Autotest {
namespace Internal {

class QuickTestFunctionSpec
{
public:
    QString m_functionName;
    TestCodeLocationAndType m_locationAndType;
};

class QuickTestCaseSpec
{
public:
    QString m_caseName;
    TestCodeLocationAndType m_locationAndType;
    QVector<QuickTestFunctionSpec> m_functions;
};

class TestQmlVisitor : public QmlJS::AST::Visitor
{
public:
    explicit TestQmlVisitor(QmlJS::Document::Ptr doc, const QmlJS::Snapshot &snapshot);

    bool visit(QmlJS::AST::UiObjectDefinition *ast) override;
    void endVisit(QmlJS::AST::UiObjectDefinition *ast) override;
    bool visit(QmlJS::AST::ExpressionStatement *ast) override;
    bool visit(QmlJS::AST::UiScriptBinding *ast) override;
    void endVisit(QmlJS::AST::UiScriptBinding *ast) override;
    bool visit(QmlJS::AST::FunctionDeclaration *ast) override;
    bool visit(QmlJS::AST::StringLiteral *ast) override;

    void throwRecursionDepthError() override;

    QVector<QuickTestCaseSpec> testCases() const { return m_testCases; }
    bool isValid() const { return !m_testCases.isEmpty(); }

private:
    QmlJS::Document::Ptr m_currentDoc;
    QmlJS::Snapshot m_snapshot;
    QStack<QuickTestCaseSpec> m_caseParseStack;
    QVector<QuickTestCaseSpec> m_testCases;
    QStack<bool> m_objectIsTestStack;
    bool m_expectTestCaseName = false;
};

class QuickTestAstVisitor : public CPlusPlus::ASTVisitor
{
public:
    QuickTestAstVisitor(CPlusPlus::Document::Ptr doc, const CPlusPlus::Snapshot &snapshot);

    bool visit(CPlusPlus::CallAST *ast) override;

    QString testBaseName() const { return m_testBaseName; }
private:
    QString m_testBaseName;
    CPlusPlus::Document::Ptr m_currentDoc;
    CPlusPlus::Snapshot m_snapshot;
};

} // namespace Internal
} // namespace Autotest
