/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSBIND_H
#define QMLJSBIND_H

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsvalueowner.h>

#include <QHash>
#include <QCoreApplication>

namespace QmlJS {

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

    bool visit(AST::UiProgram *ast) Q_DECL_OVERRIDE;
    bool visit(AST::Program *ast) Q_DECL_OVERRIDE;

    // Ui
    bool visit(AST::UiImport *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiPublicMember *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiObjectDefinition *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiObjectBinding *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiScriptBinding *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiArrayBinding *ast) Q_DECL_OVERRIDE;

    // QML/JS
    bool visit(AST::FunctionDeclaration *ast) Q_DECL_OVERRIDE;
    bool visit(AST::FunctionExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::VariableDeclaration *ast) Q_DECL_OVERRIDE;

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

#endif // QMLJSBIND_H
