/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

    QStringList includedScripts() const;
    QStringList localImports() const;

protected:
    using AST::Visitor::visit;

    static QString toString(AST::UiQualifiedId *qualifiedId, QChar delimiter = QChar('.'));
    AST::ExpressionNode *expression(AST::UiScriptBinding *ast) const;
    void processScript(AST::UiQualifiedId *qualifiedId, AST::UiObjectInitializer *initializer);

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

protected:
    Interpreter::ObjectValue *switchObjectValue(Interpreter::ObjectValue *newObjectValue);
    Interpreter::ObjectValue *bindObject(AST::UiQualifiedId *qualifiedTypeNameId, AST::UiObjectInitializer *initializer);

private:
    Document *_doc;
    Interpreter::Engine _interp;

    Interpreter::ObjectValue *_currentObjectValue;
    Interpreter::ObjectValue *_idEnvironment;
    Interpreter::ObjectValue *_functionEnvironment;
    Interpreter::ObjectValue *_rootObjectValue;

    QHash<AST::Node *, Interpreter::ObjectValue *> _qmlObjects;
    QStringList _includedScripts;
    QStringList _localImports;

    friend class Link;
};

} // end of namespace Qml

#endif // QMLBIND_H
