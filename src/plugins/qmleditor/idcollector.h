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

#ifndef IDCOLLECTOR_H
#define IDCOLLECTOR_H

#include <QMap>
#include <QPair>
#include <QStack>
#include <QString>

#include "qmljsastvisitor_p.h"
#include "qmlsymbol.h"

namespace QmlEditor {
namespace Internal {

class IdCollector: protected QmlJS::AST::Visitor
{
public:
    QMap<QString, QmlIdSymbol*> operator()(const QString &fileName, QmlJS::AST::UiProgram *ast);

protected:
    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);
    virtual bool visit(QmlJS::AST::UiScriptBinding *ast);

    virtual void endVisit(QmlJS::AST::UiObjectBinding *);
    virtual void endVisit(QmlJS::AST::UiObjectDefinition *);

private:
    void addId(const QString &id, QmlJS::AST::UiScriptBinding *ast);

private:
    QString _fileName;
    QMap<QString, QmlIdSymbol*> _ids;
    QStack<QmlJS::AST::Node *> _scopes;
};

} // namespace Internal
} // namespace QmlEditor

#endif // IDCOLLECTOR_H
