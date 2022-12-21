// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "import.h"
#include "qmlrewriter.h"

namespace QmlDesigner {
namespace Internal {

class ChangeImportsVisitor: public QMLRewriter
{
public:
    ChangeImportsVisitor(QmlDesigner::TextModifier &textModifier, const QString &source);

    bool add(QmlJS::AST::UiProgram *ast, const Import &import);
    bool remove(QmlJS::AST::UiProgram *ast, const Import &import);

private:
    static bool equals(QmlJS::AST::UiImport *ast, const Import &import);

    QString m_source;
};

} // namespace Internal
} // namespace QmlDesigner
