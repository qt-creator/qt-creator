/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QMLBIND_H
#define QMLBIND_H

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsinterpreter.h>

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>

namespace QmlJS {

class Link;
class Document;

class QMLJS_EXPORT Bind: protected AST::Visitor
{
    Q_DISABLE_COPY(Bind)

public:
    Bind(Document *doc);
    virtual ~Bind();

    QStringList fileImports() const;
    QStringList directoryImports() const;
    QStringList libraryImports() const;

    Interpreter::ObjectValue *currentObjectValue() const;
    Interpreter::ObjectValue *idEnvironment() const;
    Interpreter::ObjectValue *rootObjectValue() const;

    Interpreter::ObjectValue *findQmlObject(AST::Node *node) const;
    bool usesQmlPrototype(Interpreter::ObjectValue *prototype,
                          Interpreter::Context *context) const;

    Interpreter::ObjectValue *findFunctionScope(AST::FunctionDeclaration *node) const;
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
    virtual bool visit(AST::VariableDeclaration *ast);

    Interpreter::ObjectValue *switchObjectValue(Interpreter::ObjectValue *newObjectValue);
    Interpreter::ObjectValue *bindObject(AST::UiQualifiedId *qualifiedTypeNameId, AST::UiObjectInitializer *initializer);

    AST::ExpressionNode *expression(AST::UiScriptBinding *ast) const;

private:
    Document *_doc;
    Interpreter::Engine _engine;

    Interpreter::ObjectValue *_currentObjectValue;
    Interpreter::ObjectValue *_idEnvironment;
    Interpreter::ObjectValue *_rootObjectValue;

    QHash<AST::Node *, Interpreter::ObjectValue *> _qmlObjects;
    QSet<AST::Node *> _groupedPropertyBindings;
    QHash<AST::FunctionDeclaration *, Interpreter::ObjectValue *> _functionScopes;
    QStringList _includedScripts;

    QStringList _fileImports;
    QStringList _directoryImports;
    QStringList _libraryImports;
};

} // end of namespace Qml

#endif // QMLBIND_H
