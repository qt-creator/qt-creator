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

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>

namespace Autotest {
namespace Internal {

class TestQmlVisitor : public QmlJS::AST::Visitor
{
public:
    explicit TestQmlVisitor(QmlJS::Document::Ptr doc);

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
