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

#ifndef QMLIDCOLLECTOR_H
#define QMLIDCOLLECTOR_H

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/parser/qmljsengine_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljssymbol.h>

#include <QMap>
#include <QPair>
#include <QStack>
#include <QString>

namespace QmlJS {
namespace Internal {

class QMLJS_EXPORT IdCollector: protected AST::Visitor
{
public:
    QMap<QString, IdSymbol*> operator()(Document &doc);

    QList<DiagnosticMessage> diagnosticMessages()
    { return _diagnosticMessages; }

protected:
    virtual bool visit(AST::UiArrayBinding *ast);
    virtual bool visit(AST::UiObjectBinding *ast);
    virtual bool visit(AST::UiObjectDefinition *ast);
    virtual bool visit(AST::UiScriptBinding *ast);

private:
    SymbolFromFile *switchSymbol(AST::UiObjectMember *node);
    void addId(const QString &id, AST::UiScriptBinding *ast);

private:
    Document *_doc;
    QMap<QString, IdSymbol*> _ids;
    SymbolFromFile *_currentSymbol;
    QList<DiagnosticMessage> _diagnosticMessages;
};

} // namespace Internal
} // namespace Qml

#endif // QMLIDCOLLECTOR_H
