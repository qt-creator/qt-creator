/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLBIND_H
#define QMLBIND_H

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsinterpreter.h>

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>
#include <QtCore/QCoreApplication>

namespace QmlJS {

class Link;
class Document;

class QMLJS_EXPORT Bind: protected AST::Visitor
{
    Q_DISABLE_COPY(Bind)
    Q_DECLARE_TR_FUNCTIONS(QmlJS::Bind)

public:
    Bind(Document *doc, QList<DiagnosticMessage> *messages);
    virtual ~Bind();

    QList<Interpreter::ImportInfo> imports() const;

    Interpreter::ObjectValue *idEnvironment() const;
    Interpreter::ObjectValue *rootObjectValue() const;

    Interpreter::ObjectValue *findQmlObject(AST::Node *node) const;
    bool usesQmlPrototype(Interpreter::ObjectValue *prototype,
                          const Interpreter::Context *context) const;

    Interpreter::ObjectValue *findFunctionScope(AST::FunctionExpression *node) const;
    bool isGroupedPropertyBinding(AST::Node *node) const;

    static QString toString(AST::UiQualifiedId *qualifiedId, QChar delimiter = QChar('.'));

protected:
    using AST::Visitor::visit;

    void accept(AST::Node *node);

    virtual bool visit(AST::UiProgram *ast);
    virtual bool visit(AST::Program *ast);

    // Ui
    virtual bool visit(AST::UiImport *ast);
    virtual bool visit(AST::UiPublicMember *ast);
    virtual bool visit(AST::UiObjectDefinition *ast);
    virtual bool visit(AST::UiObjectBinding *ast);
    virtual bool visit(AST::UiScriptBinding *ast);
    virtual bool visit(AST::UiArrayBinding *ast);

    // QML/JS
    virtual bool visit(AST::FunctionDeclaration *ast);
    virtual bool visit(AST::FunctionExpression *ast);
    virtual bool visit(AST::VariableDeclaration *ast);

    Interpreter::ObjectValue *switchObjectValue(Interpreter::ObjectValue *newObjectValue);
    Interpreter::ObjectValue *bindObject(AST::UiQualifiedId *qualifiedTypeNameId, AST::UiObjectInitializer *initializer);

private:
    Document *_doc;
    Interpreter::Engine _engine;

    Interpreter::ObjectValue *_currentObjectValue;
    Interpreter::ObjectValue *_idEnvironment;
    Interpreter::ObjectValue *_rootObjectValue;

    QHash<AST::Node *, Interpreter::ObjectValue *> _qmlObjects;
    QSet<AST::Node *> _groupedPropertyBindings;
    QHash<AST::FunctionExpression *, Interpreter::ObjectValue *> _functionScopes;
    QStringList _includedScripts;

    QList<Interpreter::ImportInfo> _imports;

    QList<DiagnosticMessage> *_diagnosticMessages;
};

} // end of namespace Qml

#endif // QMLBIND_H
