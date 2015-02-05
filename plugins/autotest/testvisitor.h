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

#ifndef TESTVISITOR_H
#define TESTVISITOR_H

#include "testtreeitem.h"

#include <cplusplus/ASTVisitor.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/Scope.h>
#include <cplusplus/SymbolVisitor.h>

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

    bool visit(CPlusPlus::Class *symbol);

private:
    QString m_className;
    QMap<QString, TestCodeLocationAndType> m_privSlots;
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
    CPlusPlus::Scope *m_currentScope;
    CPlusPlus::Document::Ptr m_currentDoc;

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

} // namespace Internal
} // namespace Autotest

#endif // TESTVISITOR_H
