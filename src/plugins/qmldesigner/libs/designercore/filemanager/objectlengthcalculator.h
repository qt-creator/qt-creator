// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>

namespace QmlDesigner {

class ObjectLengthCalculator: protected QmlJS::AST::Visitor
{
public:
    ObjectLengthCalculator();

    bool operator()(const QString &text, quint32 offset, quint32 &length);

protected:
    using QmlJS::AST::Visitor::visit;

    bool visit(QmlJS::AST::UiObjectBinding *ast) override;
    bool visit(QmlJS::AST::UiObjectDefinition *ast) override;

    void throwRecursionDepthError() override;

private:
    QmlJS::Document::MutablePtr m_doc;
    quint32 m_offset = 0;
    quint32 m_length = 0;
};

} // namespace QmlDesigner
