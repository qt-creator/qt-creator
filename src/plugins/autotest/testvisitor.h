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

#ifndef TESTVISITOR_H
#define TESTVISITOR_H

#include "testtreeitem.h"

#include <cplusplus/ASTVisitor.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Scope.h>
#include <cplusplus/SymbolVisitor.h>

#include <cpptools/symbolfinder.h>

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>

#include <QMap>
#include <QString>

namespace Autotest {
namespace Internal {

class TestVisitor : public CPlusPlus::SymbolVisitor
{
public:
    TestVisitor(const QString &fullQualifiedClassName);
    virtual ~TestVisitor();

    QMap<QString, TestCodeLocationAndType> privateSlots() const { return m_privSlots; }
    bool resultValid() const { return m_valid; }

    bool visit(CPlusPlus::Class *symbol);

private:
    CppTools::SymbolFinder m_symbolFinder;
    QString m_className;
    QMap<QString, TestCodeLocationAndType> m_privSlots;
    bool m_valid = false;
};

class TestAstVisitor : public CPlusPlus::ASTVisitor
{
public:
    TestAstVisitor(CPlusPlus::Document::Ptr doc);
    virtual ~TestAstVisitor();

    bool visit(CPlusPlus::CallAST *ast);
    bool visit(CPlusPlus::CompoundStatementAST *ast);

    QString className() const { return m_className; }

private:
    QString m_className;
    CPlusPlus::Scope *m_currentScope = 0;
    CPlusPlus::Document::Ptr m_currentDoc;

};

class TestDataFunctionVisitor : public CPlusPlus::ASTVisitor
{
public:
    TestDataFunctionVisitor(CPlusPlus::Document::Ptr doc);
    virtual ~TestDataFunctionVisitor();

    bool visit(CPlusPlus::UsingDirectiveAST *ast);
    bool visit(CPlusPlus::FunctionDefinitionAST *ast);
    bool visit(CPlusPlus::CallAST *ast);
    bool preVisit(CPlusPlus::AST *ast);
    void postVisit(CPlusPlus::AST *ast);
    QMap<QString, TestCodeLocationList> dataTags() const { return m_dataTags; }

private:
    QString extractNameFromAST(CPlusPlus::StringLiteralAST *ast, bool *ok) const;
    bool newRowCallFound(CPlusPlus::CallAST *ast, unsigned *firstToken) const;

    CPlusPlus::Document::Ptr m_currentDoc;
    CPlusPlus::Overview m_overview;
    QString m_currentFunction;
    QMap<QString, TestCodeLocationList> m_dataTags;
    TestCodeLocationList m_currentTags;
    unsigned m_currentAstDepth;
    unsigned m_insideUsingQTestDepth;
    bool m_insideUsingQTest;
};

class TestQmlVisitor : public QmlJS::AST::Visitor
{
public:
    TestQmlVisitor(QmlJS::Document::Ptr doc);
    virtual ~TestQmlVisitor();

    bool visit(QmlJS::AST::UiObjectDefinition *ast);
    bool visit(QmlJS::AST::ExpressionStatement *ast);
    bool visit(QmlJS::AST::UiScriptBinding *ast);
    bool visit(QmlJS::AST::FunctionDeclaration *ast);
    bool visit(QmlJS::AST::StringLiteral *ast);

    QString testCaseName() const { return m_currentTestCaseName; }
    TestCodeLocationAndType testCaseLocation() const { return m_testCaseLocation; }
    QMap<QString, TestCodeLocationAndType> testFunctions() const { return m_testFunctions; }

private:
    QmlJS::Document::Ptr m_currentDoc;
    QString m_currentTestCaseName;
    TestCodeLocationAndType m_testCaseLocation;
    QMap<QString, TestCodeLocationAndType> m_testFunctions;

};

inline bool operator<(const GTestCaseSpec &spec1, const GTestCaseSpec &spec2)
{
    if (spec1.testCaseName != spec2.testCaseName)
        return spec1.testCaseName < spec2.testCaseName;
    if (spec1.parameterized == spec2.parameterized) {
        if (spec1.typed == spec2.typed) {
            if (spec1.disabled == spec2.disabled)
                return false;
            else
                return !spec1.disabled;
        } else {
            return !spec1.typed;
        }
    } else {
        return !spec1.parameterized;
    }
}

class GTestVisitor : public CPlusPlus::ASTVisitor
{
public:
    GTestVisitor(CPlusPlus::Document::Ptr doc);
    bool visit(CPlusPlus::FunctionDefinitionAST *ast);

    QMap<GTestCaseSpec, TestCodeLocationList> gtestFunctions() const { return m_gtestFunctions; }

private:
    CPlusPlus::Document::Ptr m_document;
    CPlusPlus::Overview m_overview;
    QMap<GTestCaseSpec, TestCodeLocationList> m_gtestFunctions;

};

} // namespace Internal
} // namespace Autotest

#endif // TESTVISITOR_H
