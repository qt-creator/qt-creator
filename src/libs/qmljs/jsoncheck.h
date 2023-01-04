// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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

    bool visit(AST::TemplateLiteral *ast) override;
    bool visit(AST::ObjectPattern *ast) override;
    bool visit(AST::ArrayPattern *ast) override;
    bool visit(AST::NullExpression *ast) override;
    bool visit(AST::TrueLiteral *ast) override;
    bool visit(AST::FalseLiteral *ast) override;
    bool visit(AST::NumericLiteral *ast) override;
    bool visit(AST::StringLiteral *ast) override;

    void throwRecursionDepthError() override;

    struct AnalysisData
    {
        AnalysisData() : m_ranking(0), m_hasMatch(false) {}

        void boostRanking(int unit = 1) { m_ranking += unit; }

        int m_ranking;
        bool m_hasMatch;
        QList<StaticAnalysis::Message> m_messages;
    };

    void processSchema(AST::Node *ast);
    bool proceedCheck(Utils::JsonValue::Kind kind, const SourceLocation &location);

    AnalysisData *analysis();

    Document::Ptr m_doc;
    SourceLocation m_firstLoc;
    Utils::JsonSchema *m_schema;
    QStack<AnalysisData> m_analysis;
};

} // QmlJs
