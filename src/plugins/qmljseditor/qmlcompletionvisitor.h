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

#ifndef COMPLETIONVISITOR_H
#define COMPLETIONVISITOR_H

#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QStack>
#include <QtCore/QString>

#include "qmljsastfwd_p.h"
#include "qmljsastvisitor_p.h"
#include "qmljsengine_p.h"

namespace QmlEditor {
namespace Internal {

class QmlCompletionVisitor: protected QmlJS::AST::Visitor
{
public:
    QmlCompletionVisitor();

    QSet<QString> operator()(QmlJS::AST::UiProgram *ast, int pos);

protected:
    virtual bool preVisit(QmlJS::AST::Node *node);
    virtual void postVisit(QmlJS::AST::Node *) { m_parentStack.pop(); }

    virtual bool visit(QmlJS::AST::UiScriptBinding *ast);

private:
    QmlJS::AST::UiObjectDefinition *findParentObject(QmlJS::AST::Node *node) const;

private:
    QSet<QString> m_completions;
    quint32 m_pos;
    QStack<QmlJS::AST::Node *> m_parentStack;
    QMap<QmlJS::AST::Node *, QmlJS::AST::Node *> m_nodeParents;
    QMap<QmlJS::AST::Node *, QString> m_objectToId;
};

} // namespace Internal
} // namespace QmlEditor

#endif // COMPLETIONVISITOR_H
