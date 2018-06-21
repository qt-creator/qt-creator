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

#include "qttesttreeitem.h"

#include <cplusplus/ASTVisitor.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Scope.h>
#include <cplusplus/SymbolVisitor.h>
#include <cpptools/symbolfinder.h>

#include <QMap>
#include <QSet>

namespace Autotest {
namespace Internal {

class TestVisitor : public CPlusPlus::SymbolVisitor
{
public:
    explicit TestVisitor(const QString &fullQualifiedClassName, const CPlusPlus::Snapshot &snapshot);

    void setInheritedMode(bool inherited) { m_inherited = inherited; }
    QMap<QString, QtTestCodeLocationAndType> privateSlots() const { return m_privSlots; }
    QSet<QString> baseClasses() const { return m_baseClasses; }
    bool resultValid() const { return m_valid; }

    bool visit(CPlusPlus::Class *symbol);

private:
    CppTools::SymbolFinder m_symbolFinder;
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

    bool visit(CPlusPlus::CallAST *ast);
    bool visit(CPlusPlus::CompoundStatementAST *ast);

    QString className() const { return m_className; }

private:
    QString m_className;
    CPlusPlus::Scope *m_currentScope = nullptr;
    CPlusPlus::Document::Ptr m_currentDoc;
    CPlusPlus::Snapshot m_snapshot;
};

class TestDataFunctionVisitor : public CPlusPlus::ASTVisitor
{
public:
    explicit TestDataFunctionVisitor(CPlusPlus::Document::Ptr doc);

    bool visit(CPlusPlus::UsingDirectiveAST *ast);
    bool visit(CPlusPlus::FunctionDefinitionAST *ast);
    bool visit(CPlusPlus::CallAST *ast);
    bool preVisit(CPlusPlus::AST *ast);
    void postVisit(CPlusPlus::AST *ast);
    QMap<QString, QtTestCodeLocationList> dataTags() const { return m_dataTags; }

private:
    QString extractNameFromAST(CPlusPlus::StringLiteralAST *ast, bool *ok) const;
    bool newRowCallFound(CPlusPlus::CallAST *ast, unsigned *firstToken) const;

    CPlusPlus::Document::Ptr m_currentDoc;
    CPlusPlus::Overview m_overview;
    QString m_currentFunction;
    QMap<QString, QtTestCodeLocationList> m_dataTags;
    QtTestCodeLocationList m_currentTags;
    unsigned m_currentAstDepth = 0;
    unsigned m_insideUsingQTestDepth = 0;
    bool m_insideUsingQTest = false;
};

} // namespace Internal
} // namespace Autotest
