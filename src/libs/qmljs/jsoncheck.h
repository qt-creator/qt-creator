/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
    bool preVisit(AST::Node *) override;
    void postVisit(AST::Node *) override;

    bool visit(AST::ObjectLiteral *ast) override;
    bool visit(AST::ArrayLiteral *ast) override;
    bool visit(AST::NullExpression *ast) override;
    bool visit(AST::TrueLiteral *ast) override;
    bool visit(AST::FalseLiteral *ast) override;
    bool visit(AST::NumericLiteral *ast) override;
    bool visit(AST::StringLiteral *ast) override;

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
