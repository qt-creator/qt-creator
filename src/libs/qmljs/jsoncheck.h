/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef JSONCHECK_H
#define JSONCHECK_H

#include "qmljs_global.h"

#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsstaticanalysismessage.h>

#include <utils/json.h>

#include <QtCore/QString>
#include <QtCore/QSet>
#include <QtCore/QStack>
#include <QtCore/QList>


namespace QmlJS {

class QMLJS_EXPORT JsonCheck : public AST::Visitor
{
public:
    JsonCheck(Document::Ptr doc);
    virtual ~JsonCheck();

    QList<StaticAnalysis::Message> operator()(Utils::JsonSchema *schema);

private:
    virtual bool preVisit(AST::Node *);
    virtual void postVisit(AST::Node *);

    virtual bool visit(AST::ObjectLiteral *ast);
    virtual bool visit(AST::ArrayLiteral *ast);
    virtual bool visit(AST::NullExpression *ast);
    virtual bool visit(AST::TrueLiteral *ast);
    virtual bool visit(AST::FalseLiteral *ast);
    virtual bool visit(AST::NumericLiteral *ast);
    virtual bool visit(AST::StringLiteral *ast);

    struct AnalysisData
    {
        AnalysisData() : m_ranking(0), m_hasMatch(false), m_evaluatinType(false) {}

        void boostRanking(int unit = 1) { m_ranking += unit; }

        int m_ranking;
        bool m_hasMatch;
        bool m_evaluatinType;
        QList<StaticAnalysis::Message> m_messages;
    };

    void processSchema(AST::Node *ast);
    bool proceedCheck(Utils::JsonValue::Kind kind, const AST::SourceLocation &location);

    AnalysisData *analysis();

    Document::Ptr m_doc;
    AST::SourceLocation m_firstLoc;
    Utils::JsonSchema *m_schema;
    QStack<AnalysisData> m_analysis;
};

} // QmlJs

#endif // JSONCHECK_H
