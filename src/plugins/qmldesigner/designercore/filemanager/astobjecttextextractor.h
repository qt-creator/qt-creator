// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>

#include <QString>

namespace QmlDesigner {

class ASTObjectTextExtractor: public QmlJS::AST::Visitor
{
public:
    ASTObjectTextExtractor(const QString &text);

    QString operator()(int location);

protected:
    bool visit(QmlJS::AST::UiObjectBinding *ast) override;
    bool visit(QmlJS::AST::UiObjectDefinition *ast) override;

    void throwRecursionDepthError() override;

private:
    QmlJS::Document::MutablePtr m_document;
    quint32 m_location = 0;
    QString m_text;
};

} // namespace QmlDesigner
