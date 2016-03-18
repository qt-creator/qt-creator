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

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsvalueowner.h>

#include <QHash>
#include <QCoreApplication>

namespace QmlJS {

class DiagnosticMessage;
class Document;

class QMLJS_EXPORT Bind: protected AST::Visitor
{
    Q_DISABLE_COPY(Bind)
    Q_DECLARE_TR_FUNCTIONS(QmlJS::Bind)

public:
    Bind(Document *doc, QList<DiagnosticMessage> *messages,
         bool isJsLibrary, const QList<ImportInfo> &jsImports);
    ~Bind();

    bool isJsLibrary() const;
    QList<ImportInfo> imports() const;

    ObjectValue *idEnvironment() const;
    ObjectValue *rootObjectValue() const;

    ObjectValue *findQmlObject(AST::Node *node) const;
    bool usesQmlPrototype(ObjectValue *prototype,
                          const ContextPtr &context) const;

    ObjectValue *findAttachedJSScope(AST::Node *node) const;
    bool isGroupedPropertyBinding(AST::Node *node) const;

protected:
    using AST::Visitor::visit;

    void accept(AST::Node *node);

    bool visit(AST::UiProgram *ast) override;
    bool visit(AST::Program *ast) override;

    // Ui
    bool visit(AST::UiImport *ast) override;
    bool visit(AST::UiPublicMember *ast) override;
    bool visit(AST::UiObjectDefinition *ast) override;
    bool visit(AST::UiObjectBinding *ast) override;
    bool visit(AST::UiScriptBinding *ast) override;
    bool visit(AST::UiArrayBinding *ast) override;

    // QML/JS
    bool visit(AST::FunctionDeclaration *ast) override;
    bool visit(AST::FunctionExpression *ast) override;
    bool visit(AST::VariableDeclaration *ast) override;

    ObjectValue *switchObjectValue(ObjectValue *newObjectValue);
    ObjectValue *bindObject(AST::UiQualifiedId *qualifiedTypeNameId, AST::UiObjectInitializer *initializer);

private:
    Document *_doc;
    ValueOwner _valueOwner;

    ObjectValue *_currentObjectValue;
    ObjectValue *_idEnvironment;
    ObjectValue *_rootObjectValue;

    QHash<AST::Node *, ObjectValue *> _qmlObjects;
    QMultiHash<QString, const ObjectValue *> _qmlObjectsByPrototypeName;
    QSet<AST::Node *> _groupedPropertyBindings;
    QHash<AST::Node *, ObjectValue *> _attachedJSScopes;

    bool _isJsLibrary;
    QList<ImportInfo> _imports;

    QList<DiagnosticMessage> *_diagnosticMessages;
};

} // namespace Qml
