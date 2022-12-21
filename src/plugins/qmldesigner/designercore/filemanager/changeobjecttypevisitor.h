// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlrewriter.h"

namespace QmlDesigner {
namespace Internal {

class ChangeObjectTypeVisitor: public QMLRewriter
{
public:
    ChangeObjectTypeVisitor(QmlDesigner::TextModifier &modifier,
                            quint32 nodeLocation,
                            const QString &newType);

protected:
    bool visit(QmlJS::AST::UiObjectDefinition *ast) override;
    bool visit(QmlJS::AST::UiObjectBinding *ast) override;

private:
    void replaceType(QmlJS::AST::UiQualifiedId *typeId);

private:
    quint32 m_nodeLocation;
    QString m_newType;
};

} // namespace Internal
} // namespace QmlDesigner
