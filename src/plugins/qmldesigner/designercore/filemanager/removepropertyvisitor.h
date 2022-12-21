// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

#include "qmlrewriter.h"

namespace QmlDesigner {
namespace Internal {

class RemovePropertyVisitor: public QMLRewriter
{
public:
    RemovePropertyVisitor(QmlDesigner::TextModifier &modifier,
                          quint32 parentLocation,
                          const QString &name);

protected:
    bool visit(QmlJS::AST::UiObjectBinding *ast) override;
    bool visit(QmlJS::AST::UiObjectDefinition *ast) override;

private:
    void removeFrom(QmlJS::AST::UiObjectInitializer *ast);
    static bool memberNameMatchesPropertyName(const QString &propertyName,
                                              QmlJS::AST::UiObjectMember *ast);
    void removeGroupedProperty(QmlJS::AST::UiObjectDefinition *ast);
    void removeMember(QmlJS::AST::UiObjectMember *ast);

private:
    quint32 parentLocation;
    QString propertyName;
};

} // namespace Internal
} // namespace QmlDesigner
