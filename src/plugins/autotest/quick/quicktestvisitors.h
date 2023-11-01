// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "quicktesttreeitem.h"

#include <cplusplus/ASTVisitor.h>
#include <cplusplus/CppDocument.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>

#include <QStack>

namespace Autotest {
namespace Internal {

class QuickTestCaseSpec
{
public:
    QString m_caseName;
    TestCodeLocationAndType m_locationAndType;
    TestCodeLocationList m_functions;
};

class TestQmlVisitor : public QmlJS::AST::Visitor
{
public:
    TestQmlVisitor(QmlJS::Document::Ptr doc,
                   const QmlJS::Snapshot &snapshot,
                   bool checkForDerivedTest);

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
    const QmlJS::Snapshot &m_snapshot;
    QStack<QuickTestCaseSpec> m_caseParseStack;
    QVector<QuickTestCaseSpec> m_testCases;
    QStack<bool> m_objectIsTestStack;
    bool m_expectTestCaseName = false;
    bool m_checkForDerivedTest = false;
};

class QuickTestAstVisitor : public CPlusPlus::ASTVisitor
{
public:
    QuickTestAstVisitor(CPlusPlus::Document::Ptr doc);

    bool visit(CPlusPlus::CallAST *ast) override;

    QString testBaseName() const { return m_testBaseName; }
private:
    QString m_testBaseName;
    CPlusPlus::Document::Ptr m_currentDoc;
};

} // namespace Internal
} // namespace Autotest
