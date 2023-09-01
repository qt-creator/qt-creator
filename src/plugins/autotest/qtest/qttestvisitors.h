// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qttest_utils.h"
#include "qttesttreeitem.h"

#include <cplusplus/ASTVisitor.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Scope.h>
#include <cplusplus/SymbolVisitor.h>
#include <cppeditor/symbolfinder.h>

#include <QMap>
#include <QSet>

namespace Autotest::Internal {

class TestVisitor : public CPlusPlus::SymbolVisitor
{
public:
    explicit TestVisitor(const QString &fullQualifiedClassName, const CPlusPlus::Snapshot &snapshot);

    void setInheritedMode(bool inherited) { m_inherited = inherited; }
    QMap<QString, QtTestCodeLocationAndType> privateSlots() const { return m_privSlots; }
    QSet<QString> baseClasses() const { return m_baseClasses; }
    bool resultValid() const { return m_valid; }

    bool visit(CPlusPlus::Class *symbol) override;

private:
    CppEditor::SymbolFinder m_symbolFinder;
    QString m_className;
    CPlusPlus::Snapshot m_snapshot;
    QMap<QString, QtTestCodeLocationAndType> m_privSlots;
    bool m_valid = false;
    bool m_inherited = false;
    QSet<QString> m_baseClasses;
};

class TestAstVisitor : public CPlusPlus::ASTVisitor
{
public:
    explicit TestAstVisitor(CPlusPlus::Document::Ptr doc, const CPlusPlus::Snapshot &snapshot);

    bool visit(CPlusPlus::CallAST *ast) override;
    bool visit(CPlusPlus::CompoundStatementAST *ast) override;

    TestCases testCases() const;

private:
    QStringList m_classNames;
    CPlusPlus::Scope *m_currentScope = nullptr;
    CPlusPlus::Document::Ptr m_currentDoc;
    const CPlusPlus::Snapshot &m_snapshot;
};

class TestDataFunctionVisitor : public CPlusPlus::ASTVisitor
{
public:
    explicit TestDataFunctionVisitor(CPlusPlus::Document::Ptr doc);

    bool visit(CPlusPlus::UsingDirectiveAST *ast) override;
    bool visit(CPlusPlus::FunctionDefinitionAST *ast) override;
    bool visit(CPlusPlus::CallAST *ast) override;
    bool preVisit(CPlusPlus::AST *ast) override;
    void postVisit(CPlusPlus::AST *ast) override;
    QHash<QString, QtTestCodeLocationList> dataTags() const { return m_dataTags; }

private:
    QString extractNameFromAST(CPlusPlus::StringLiteralAST *ast, bool *ok) const;
    bool newRowCallFound(CPlusPlus::CallAST *ast, unsigned *firstToken) const;

    CPlusPlus::Document::Ptr m_currentDoc;
    CPlusPlus::Overview m_overview;
    QString m_currentFunction;
    QHash<QString, QtTestCodeLocationList> m_dataTags;
    QtTestCodeLocationList m_currentTags;
    unsigned m_currentAstDepth = 0;
    unsigned m_insideUsingQTestDepth = 0;
    bool m_insideUsingQTest = false;
};

} // namespace Autotest::Internal
