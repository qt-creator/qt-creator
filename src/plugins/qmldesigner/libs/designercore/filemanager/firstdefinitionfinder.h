// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>

namespace QmlDesigner {

class FirstDefinitionFinder: protected QmlJS::AST::Visitor
{
public:
    FirstDefinitionFinder(const QString &text);

    qint32 operator()(quint32 offset);

protected:
    using QmlJS::AST::Visitor::visit;

    bool visit(QmlJS::AST::UiObjectBinding *ast) override;
    bool visit(QmlJS::AST::UiObjectDefinition *ast) override;
    bool visit(QmlJS::AST::TemplateLiteral *ast) override;

    void throwRecursionDepthError() override;

    void extractFirstObjectDefinition(QmlJS::AST::UiObjectInitializer* ast);

private:
    QmlJS::Document::MutablePtr m_doc;
    quint32 m_offset;
    QmlJS::AST::UiObjectDefinition *m_firstObjectDefinition = nullptr;

};

} // namespace QmlDesigner
