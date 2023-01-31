// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

public:
    Bind(Document *doc, QList<DiagnosticMessage> *messages,
         bool isJsLibrary, const QList<ImportInfo> &jsImports);
    ~Bind();

    bool isJsLibrary() const;
    const QList<ImportInfo> imports() const;

    ObjectValue *idEnvironment() const;
    ObjectValue *rootObjectValue() const;

    ObjectValue *findQmlObject(AST::Node *node) const;
    bool usesQmlPrototype(ObjectValue *prototype,
                          const ContextPtr &context) const;

    ObjectValue *findAttachedJSScope(AST::Node *node) const;
    bool isGroupedPropertyBinding(AST::Node *node) const;

    QHash<QString, ObjectValue*> inlineComponents() const {
        return _inlineComponents;
    }

protected:
    using AST::Visitor::visit;

    void accept(AST::Node *node);

    bool visit(AST::UiProgram *ast) override;
    bool visit(AST::Program *ast) override;
    void endVisit(AST::UiProgram *) override;

    // Ui
    bool visit(AST::UiImport *ast) override;
    bool visit(AST::UiPublicMember *ast) override;
    bool visit(AST::UiObjectDefinition *ast) override;
    bool visit(AST::UiObjectBinding *ast) override;
    bool visit(AST::UiScriptBinding *ast) override;
    bool visit(AST::UiArrayBinding *ast) override;
    bool visit(AST::UiInlineComponent *ast) override;

    // QML/JS
    bool visit(AST::TemplateLiteral *ast) override;
    bool visit(AST::FunctionDeclaration *ast) override;
    bool visit(AST::FunctionExpression *ast) override;
    bool visit(AST::PatternElement *ast) override;

    ObjectValue *switchObjectValue(ObjectValue *newObjectValue);
    ObjectValue *bindObject(AST::UiQualifiedId *qualifiedTypeNameId, AST::UiObjectInitializer *initializer);

    void throwRecursionDepthError() override;

private:
    Document *_doc;
    ValueOwner _valueOwner;

    ObjectValue *_currentObjectValue;
    ObjectValue *_idEnvironment;
    ObjectValue *_rootObjectValue;
    QString _currentComponentName;

    QHash<AST::Node *, ObjectValue *> _qmlObjects;
    QMultiHash<QString, const ObjectValue *> _qmlObjectsByPrototypeName;
    QSet<AST::Node *> _groupedPropertyBindings;
    QHash<AST::Node *, ObjectValue *> _attachedJSScopes;
    QHash<QString, ObjectValue*> _inlineComponents;

    bool _isJsLibrary;
    QList<ImportInfo> _imports;

    QList<DiagnosticMessage> *_diagnosticMessages;
};

} // namespace Qml
