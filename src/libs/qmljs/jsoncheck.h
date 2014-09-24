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
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef JSONCHECK_H
#define JSONCHECK_H

#include "qmljs_global.h"

#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsstaticanalysismessage.h>

#include <utils/json.h>

#include <QStack>
#include <QList>


namespace QmlJS {

class QMLJS_EXPORT JsonCheck : public AST::Visitor
{
public:
    JsonCheck(Document::Ptr doc);
    ~JsonCheck();

    QList<StaticAnalysis::Message> operator()(Utils::JsonSchema *schema);

private:
    bool preVisit(AST::Node *) Q_DECL_OVERRIDE;
    void postVisit(AST::Node *) Q_DECL_OVERRIDE;

    bool visit(AST::ObjectLiteral *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ArrayLiteral *ast) Q_DECL_OVERRIDE;
    bool visit(AST::NullExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::TrueLiteral *ast) Q_DECL_OVERRIDE;
    bool visit(AST::FalseLiteral *ast) Q_DECL_OVERRIDE;
    bool visit(AST::NumericLiteral *ast) Q_DECL_OVERRIDE;
    bool visit(AST::StringLiteral *ast) Q_DECL_OVERRIDE;

    struct AnalysisData
    {
        AnalysisData() : m_ranking(0), m_hasMatch(false) {}

        void boostRanking(int unit = 1) { m_ranking += unit; }

        int m_ranking;
        bool m_hasMatch;
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
