/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMLJSCHECK_H
#define QMLJSCHECK_H

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/parser/qmljsastvisitor_p.h>

#include <QtCore/QCoreApplication>
#include <QtGui/QColor>

namespace QmlJS {

class QMLJS_EXPORT Check: protected AST::Visitor
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::Check)

public:
    Check(Document::Ptr doc, const Snapshot &snapshot, const Interpreter::Context *linkedContextNoScope);
    virtual ~Check();

    QList<DiagnosticMessage> operator()();

protected:
    virtual bool visit(AST::UiProgram *ast);
    virtual bool visit(AST::UiObjectDefinition *ast);
    virtual bool visit(AST::UiObjectBinding *ast);
    virtual bool visit(AST::UiScriptBinding *ast);
    virtual bool visit(AST::UiArrayBinding *ast);

private:
    void visitQmlObject(AST::Node *ast, AST::UiQualifiedId *typeId,
                        AST::UiObjectInitializer *initializer);
    const Interpreter::Value *checkScopeObjectMember(const AST::UiQualifiedId *id);

    void warning(const AST::SourceLocation &loc, const QString &message);
    void error(const AST::SourceLocation &loc, const QString &message);

    Document::Ptr _doc;
    Snapshot _snapshot;

    Interpreter::Context _context;
    ScopeBuilder _scopeBuilder;

    QList<DiagnosticMessage> _messages;

    bool _ignoreTypeErrors;
};

QMLJS_EXPORT QColor toQColor(const QString &qmlColorString);

QMLJS_EXPORT AST::SourceLocation locationFromRange(const AST::SourceLocation &start,
                                                   const AST::SourceLocation &end);
QMLJS_EXPORT DiagnosticMessage errorMessage(const AST::SourceLocation &loc,
                                            const QString &message);

template <class T>
DiagnosticMessage errorMessage(const T *node, const QString &message)
{
    return DiagnosticMessage(DiagnosticMessage::Error,
                             locationFromRange(node->firstSourceLocation(),
                                               node->lastSourceLocation()),
                             message);
}

} // namespace QmlJS

#endif // QMLJSCHECK_H
